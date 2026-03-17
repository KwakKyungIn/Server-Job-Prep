#include "pch.h"
#include "DBAgentPacketHandler.h"
#include "DBConnectionPool.h"
#include "DBAgentMetrics.h"
#include "GameSession.h" // [필수] GameSession 클래스를 알기 위해 추가
#include "LoginSession.h"
#include "Job.h"         // [필수] Job을 생성하기 위해 추가

static constexpr int32 QS_MAX = 12; // 0~11
PacketHandlerFunc DBAgentPacketHandler::GPacketHandler[UINT16_MAX];

bool DBAgentPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [Game -> DB] 로그인 요청 처리
bool DBAgentPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
	PacketSessionRef ownerSession = session;

	// [JOB WRAPPING] 
	// 기존 로직을 그대로 람다([]) 안으로 옮긴다.
	// 이제 이 코드는 네트워크 스레드가 아니라, 로직 스레드에서 실행된다.
	shared_ptr<Job> job = ObjectPool<Job>::MakeShared([ownerSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_LOGIN);

			// ==========================================================
			//  여기서부터 기존 코드 복사 붙여넣기 (Logic Thread 실행)
			// ==========================================================

			// 1. DB 연결 대여 (Connection Pool)
			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr)
			{
				// 연결 풀 고갈 -> 에러 처리 혹은 재시도 로직 필요
				return; // 람다 내부라 return false가 아니라 그냥 return
			}

			uint64 playerId = 0;
			bool success = false;

			// 2. SQL 실행 (Scope로 묶어서 깔끔하게 관리)
			{
				// [Step 1] 기존 바인딩 찌꺼기 청소 (필수)
				conn->Unbind();

				// [Step 2] 입력 데이터 준비 (UTF-8 string -> WCHAR 변환)
				WString wbName;
				wbName.assign(pkt.name().begin(), pkt.name().end());

				SQLLEN nameLen = SQL_NTS; // Null Terminated String

				// [Step 3] 출력 데이터 준비
				int64 outId = 0;
				SQLLEN outIdLen = 0;

				// [Step 4] 쿼리 준비 (Prepare)
				if (conn->Prepare(L"SELECT playerId FROM Players WHERE name = ?"))
				{
					// [Step 5] 파라미터 바인딩
					conn->BindParam(1, SQL_C_WCHAR, SQL_WVARCHAR, (wbName.size() + 1) * sizeof(WCHAR), (SQLPOINTER)wbName.c_str(), &nameLen);

					// [Step 6] 결과 컬럼 바인딩
					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &outId, &outIdLen);

					// [Step 7] 실행 (Execute) - 여기가 제일 느림 (Blocking)
					if (conn->Execute())
					{
						// [Step 8] 결과 인출 (Fetch)
						if (conn->Fetch())
						{
							success = true;
							playerId = outId;
							std::cout << " [DB] Login Success! Name: " << pkt.name() << " ID: " << playerId << std::endl;
						}
						else
						{
							std::cout << " [DB] User Not Found: " << pkt.name() << std::endl;
							success = false;
							// TODO: CreateAccount
						}
					}
				}
			}

			// 3. 사용한 DB 연결 반납 (필수)
			GDBConnectionPool->Push(conn);

			// 4. 응답 패킷 전송
			Protocol::S2S_RES_LOGIN resPkt;
			resPkt.set_success(success);
			resPkt.set_playerid(playerId);
			resPkt.set_playersessionid(pkt.playersessionid()); // 왕복 티켓

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			ownerSession->Send(sendBuffer);

			// ==========================================================
			//  기존 코드 끝
			// ==========================================================
		});

	if (auto gameSession = dynamic_pointer_cast<GameSession>(session))
	{
		gameSession->PushJob(job);
		return true;
	}

	if (auto loginSession = dynamic_pointer_cast<LoginSession>(session))
	{
		loginSession->PushJob(job);
		return true;
	}

	return false;
}

// [Game -> DB] 아이템 로딩 요청 처리
bool DBAgentPacketHandler::Handle_S2S_REQ_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_REQ_ITEMS_LOAD& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushHighJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_ITEMS_LOAD);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr) return;

			Protocol::S2S_RES_ITEMS_LOAD resPkt;
			resPkt.set_playerid(pkt.playerid());
			resPkt.set_gamesessionid(pkt.gamesessionid()); // 왕복 티켓
			resPkt.set_success(false);

			{
				conn->Unbind();

				// [Input] Owner ID 바인딩
				int32 ownerId = (int64)pkt.playerid(); // PlayerID가 DB에서 INT인지 BIGINT인지 확인 필요. 네 DB 설계상 FK는 INT였으면 casting.
				// ※ 주의: 네 Player 테이블 ID가 BIGINT라면 여기도 int64로 받아야 함. 
				// 아까 스키마에서 owner_id INT라고 했지만, 네가 BIGINT로 바꿨다고 했으니 아래처럼 수정한다.
				int64 dbOwnerId = (int64)pkt.playerid();

				// [Output] 결과를 담을 변수들
				int64 outItemUid = 0;
				int32 outTemplateId = 0;
				int32 outSlot = 0;
				int32 outCount = 0;
				unsigned char outEquipped = 0; // BIT 타입은 unsigned char로 받음
				int32 outEnchant = 0;

				SQLLEN len = 0;

				// [Query]
				if (conn->Prepare(L"SELECT game_item_uid, template_id, slot_index, count, is_equipped FROM ITEMS WHERE owner_id = ?"))
				{
					conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbOwnerId, &len);

					conn->BindCol(1, SQL_C_SBIGINT, sizeof(int64), &outItemUid, &len);
					conn->BindCol(2, SQL_C_SLONG, sizeof(int32), &outTemplateId, &len);
					conn->BindCol(3, SQL_C_SLONG, sizeof(int32), &outSlot, &len);
					conn->BindCol(4, SQL_C_SLONG, sizeof(int32), &outCount, &len);
					conn->BindCol(5, SQL_C_BIT, sizeof(unsigned char), &outEquipped, &len);

					if (conn->Execute())
					{
						resPkt.set_success(true);
						while (conn->Fetch())
						{
							Protocol::ItemInfo* item = resPkt.add_items();
							item->set_itemuid(outItemUid);
							item->set_templateid(outTemplateId);
							item->set_slot(outSlot);
							item->set_count(outCount);
							item->set_isequipped(outEquipped != 0);
						}
						std::cout << " [DB] Loaded Items for Player: " << pkt.playerid() << " Count: " << resPkt.items_size() << std::endl;
					}
				}
			}

			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);
		}));

	return true;
}


// [Game -> DB] 기획 데이터(Stat/Item/Skill Template) 로딩 요청
bool DBAgentPacketHandler::Handle_S2S_REQ_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_GAME_DATA& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushHighJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_LOAD_GAME_DATA);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr) return;

			Protocol::S2S_RES_LOAD_GAME_DATA resPkt;
			resPkt.set_success(false);

			// ==========================================================
			// 1. STAT_TEMPLATE 로딩
			// ==========================================================
			{
				conn->Unbind();
				int32 level = 0, maxHp = 0, attack = 0, defense = 0, speed = 0;
				int64 totalExp = 0;
				SQLLEN len = 0;

				if (conn->Prepare(L"SELECT level, max_hp, attack, defense, speed, total_exp FROM STAT_TEMPLATE"))
				{
					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &level, &len);
					conn->BindCol(2, SQL_C_SLONG, sizeof(int32), &maxHp, &len);
					conn->BindCol(3, SQL_C_SLONG, sizeof(int32), &attack, &len);
					conn->BindCol(4, SQL_C_SLONG, sizeof(int32), &defense, &len);
					conn->BindCol(5, SQL_C_SLONG, sizeof(int32), &speed, &len);
					conn->BindCol(6, SQL_C_SBIGINT, sizeof(int64), &totalExp, &len);

					if (conn->Execute())
					{
						while (conn->Fetch())
						{
							auto* statData = resPkt.add_stats();
							statData->set_level(level);
							statData->set_maxhp(maxHp);
							statData->set_attack(attack);
							statData->set_defense(defense);
							statData->set_speed(speed);
							statData->set_totalexp(totalExp);
						}
						std::cout << "[DB] Loaded Stat Templates: " << resPkt.stats_size() << std::endl;
					}
				}
			}

			// ==========================================================
			// 2. ITEM_TEMPLATE 로딩
			// ==========================================================
			{
				conn->Unbind();
				int32 templateId = 0, itemType = 0, atkBonus = 0, defBonus = 0, hpBonus = 0;
				WCHAR nameBuffer[100] = { 0 };
				SQLLEN len = 0;

				if (conn->Prepare(L"SELECT template_id, name, item_type, attack_bonus, defense_bonus, hp_bonus FROM ITEM_TEMPLATE"))
				{
					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &templateId, &len);
					conn->BindCol(2, SQL_C_WCHAR, sizeof(nameBuffer), nameBuffer, &len);
					conn->BindCol(3, SQL_C_SLONG, sizeof(int32), &itemType, &len);
					conn->BindCol(4, SQL_C_SLONG, sizeof(int32), &atkBonus, &len);
					conn->BindCol(5, SQL_C_SLONG, sizeof(int32), &defBonus, &len);
					conn->BindCol(6, SQL_C_SLONG, sizeof(int32), &hpBonus, &len);

					if (conn->Execute())
					{
						while (conn->Fetch())
						{
							auto* itemData = resPkt.add_items();
							itemData->set_templateid(templateId);
							WString ws(nameBuffer);
								String s(ws.begin(), ws.end());
								// Protobuf string 필드는 std::string/const char* 기반이므로 경계에서만 변환한다.
								itemData->set_name(s.c_str());
							itemData->set_itemtype(itemType);
							itemData->set_attackbonus(atkBonus);
							itemData->set_defensebonus(defBonus);
							itemData->set_hpbonus(hpBonus);
						}
						std::cout << " [DB] Loaded Item Templates: " << resPkt.items_size() << std::endl;
					}
				}
			}

			// ==========================================================
			// 3. SKILL_TEMPLATE 로딩 [NEW]
			// ==========================================================
			// ==========================================================
			{
				conn->Unbind(); // 필수!

				// [Output Variables]
				int32 skillId = 0, cooldown = 0, damage = 0, skillType = 0, effectId = 0;
				float range = 0.0f, radius = 0.0f, angle = 0.0f;

				// [NEW]
				float projectileSpeed = 0.0f;
				int32 projectileLifeMs = 0;
				float hitRadius = 0.0f;
				uint8 stopOnHit = 1;  // BIT 받는 용도 (0/1)
				int32 maxHits = 1;

				WCHAR nameBuffer[100] = { 0 };
				SQLLEN len = 0;

				if (conn->Prepare(
					L"SELECT skill_id, name, cooldown, damage, skill_type, attack_range, radius, angle, effect_id, "
					L"projectile_speed, projectile_life_ms, hit_radius, stop_on_hit, max_hits "
					L"FROM SKILL_TEMPLATE"))
				{
					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &skillId, &len);
					conn->BindCol(2, SQL_C_WCHAR, sizeof(nameBuffer), nameBuffer, &len);
					conn->BindCol(3, SQL_C_SLONG, sizeof(int32), &cooldown, &len);
					conn->BindCol(4, SQL_C_SLONG, sizeof(int32), &damage, &len);
					conn->BindCol(5, SQL_C_SLONG, sizeof(int32), &skillType, &len);

					conn->BindCol(6, SQL_C_FLOAT, sizeof(float), &range, &len);
					conn->BindCol(7, SQL_C_FLOAT, sizeof(float), &radius, &len);
					conn->BindCol(8, SQL_C_FLOAT, sizeof(float), &angle, &len);
					conn->BindCol(9, SQL_C_SLONG, sizeof(int32), &effectId, &len);

					// [NEW] Projectile Params
					conn->BindCol(10, SQL_C_FLOAT, sizeof(float), &projectileSpeed, &len);
					conn->BindCol(11, SQL_C_SLONG, sizeof(int32), &projectileLifeMs, &len);
					conn->BindCol(12, SQL_C_FLOAT, sizeof(float), &hitRadius, &len);
					conn->BindCol(13, SQL_C_BIT, sizeof(uint8), &stopOnHit, &len);
					conn->BindCol(14, SQL_C_SLONG, sizeof(int32), &maxHits, &len);

					if (conn->Execute())
					{
						while (conn->Fetch())
						{
							auto* skillData = resPkt.add_skills();
							skillData->set_skillid(skillId);

							WString ws(nameBuffer);
								String s(ws.begin(), ws.end());
								// Protobuf string 필드는 std::string/const char* 기반이므로 경계에서만 변환한다.
								skillData->set_name(s.c_str());

							skillData->set_cooldown(cooldown);
							skillData->set_damage(damage);
							skillData->set_skilltype(static_cast<Protocol::SkillType>(skillType));

							skillData->set_range(range);
							skillData->set_radius(radius);
							skillData->set_angle(angle);
							skillData->set_effectid(effectId);

							// [NEW] Set projectile params
							skillData->set_projectilespeed(projectileSpeed);
							skillData->set_projectilelifems(projectileLifeMs);
							skillData->set_hitradius(hitRadius);
							skillData->set_stoponhit(stopOnHit != 0);
							skillData->set_maxhits(maxHits);
						}

							std::cout << "[DB] Loaded Skill Templates: " << resPkt.skills_size() << std::endl;
					}
				}
			}

			// 4. 결과 전송
			resPkt.set_success(true);
			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);
		}));

	return true;
}


bool DBAgentPacketHandler::Handle_S2S_REQ_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_PLAYER_DATA& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushHighJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_LOAD_PLAYER_DATA);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr) return;

			Protocol::S2S_RES_LOAD_PLAYER_DATA resPkt;
			resPkt.set_success(false);
			resPkt.set_playerid(pkt.playerid());
			resPkt.set_gamesessionid(pkt.gamesessionid());

			{
				conn->Unbind();

				// [Input]
				int64 dbPlayerId = (int64)pkt.playerid();
				SQLLEN len = 0;

				// [Output]
				int32 level = 1;
				int32 hp = 100;
				int64 totalExp = 0;
				int64 gold = 0;

				// [Query]
				if (conn->Prepare(L"SELECT level, hp, total_exp, gold FROM PLAYERS WHERE playerId = ?"))
				{
					conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbPlayerId, &len);

					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &level, &len);
					conn->BindCol(2, SQL_C_SLONG, sizeof(int32), &hp, &len);
					conn->BindCol(3, SQL_C_SBIGINT, sizeof(int64), &totalExp, &len);
					conn->BindCol(4, SQL_C_SBIGINT, sizeof(int64), &gold, &len);

					if (conn->Execute())
					{
						if (conn->Fetch())
						{
							resPkt.set_success(true);

							// 결과 패킷에 담기
							Protocol::StatInfo* info = resPkt.mutable_statinfo();
							info->set_level(level);
							info->set_hp(hp);
							info->set_totalexp(totalExp);
							resPkt.set_gold(gold);

							// MaxHp 등은 어차피 GameServer가 Template 보고 다시 계산함.
							// 여기서는 DB에 저장된 "현재 상태"만 넘김.
						}
					}
				}
			}

			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);

			if (resPkt.success())
				std::cout << "[DB] Loaded Player Data (Lv." << resPkt.statinfo().level() << ") ID: " << pkt.playerid() << std::endl;
		}));

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_REQ_HEART_BEAT& pkt)
{
	DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_HEART_BEAT);

	Protocol::S2S_RES_HEART_BEAT resPkt;
	auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_SAVE_PLAYER_CORE(PacketSessionRef& session, Protocol::S2S_REQ_SAVE_PLAYER_CORE& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_SAVE_PLAYER_CORE);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr) return;

			Protocol::S2S_RES_SAVE_PLAYER_CORE resPkt;
			resPkt.set_playerid(pkt.playerid());
			resPkt.set_success(false);

			bool ok = true;

			conn->Unbind();

			int64 dbPlayerId = (int64)pkt.playerid();
			int32 level = pkt.level();
			int32 hp = pkt.hp();
			int64 totalExp = (int64)pkt.totalexp();
			int64 gold = (int64)pkt.gold();
			SQLLEN len = 0;

			if (conn->Prepare(L"UPDATE PLAYERS SET level = ?, hp = ?, total_exp = ?, gold = ? WHERE playerId = ?"))
			{
				ok &= conn->BindParam(1, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &level, &len);
				ok &= conn->BindParam(2, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &hp, &len);
				ok &= conn->BindParam(3, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &totalExp, &len);
				ok &= conn->BindParam(4, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &gold, &len);
				ok &= conn->BindParam(5, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbPlayerId, &len);

				if (ok)
					ok = conn->Execute();
			}
			else ok = false;

			resPkt.set_success(ok);

			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);
		}));

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_SAVE_INVENTORY(PacketSessionRef& session, Protocol::S2S_REQ_SAVE_INVENTORY& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_SAVE_INVENTORY);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr) return;

			Protocol::S2S_RES_SAVE_INVENTORY resPkt;
			resPkt.set_playerid(pkt.playerid());
			resPkt.set_success(false);

			bool ok = true;
			int64 dbPlayerId = (int64)pkt.playerid();
			SQLLEN len = 0;

			// BEGIN TRAN
			conn->Unbind();
			ok = conn->Execute(L"BEGIN TRAN");

			// 1) DELETE tombstones
			if (ok)
			{
				for (int i = 0; i < pkt.deleteditemuids_size(); ++i)
				{
					conn->Unbind();

					int64 dbItemUid = (int64)pkt.deleteditemuids(i);

					if (!conn->Prepare(L"DELETE FROM ITEMS WHERE owner_id = ? AND game_item_uid = ?"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbPlayerId, &len);
					ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbItemUid, &len);

					if (!ok || !conn->Execute())
					{
						ok = false; break;
					}
				}
			}

			// 2) UPDATE all items snapshot
			if (ok)
			{
				for (int i = 0; i < pkt.items_size(); ++i)
				{
					conn->Unbind();

					const Protocol::ItemInfo& it = pkt.items(i);

					int64 dbItemUid = (int64)it.itemuid();
					int32 templateId = it.templateid();
					int32 slotIndex = it.slot();
					int32 count = it.count();
					unsigned char equipped = it.isequipped() ? 1 : 0;

					if (!conn->Prepare(L"UPDATE ITEMS SET template_id = ?, slot_index = ?, count = ?, is_equipped = ? WHERE owner_id = ? AND game_item_uid = ?"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
					ok &= conn->BindParam(2, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(3, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
					ok &= conn->BindParam(4, SQL_C_BIT, SQL_BIT, sizeof(unsigned char), &equipped, &len);
					ok &= conn->BindParam(5, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbPlayerId, &len);
					ok &= conn->BindParam(6, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbItemUid, &len);

					if (!ok || !conn->Execute())
					{
						ok = false; break;
					}

					// 정합성 체크(선택): 없던 uid면 0 row -> 실패 처리 가능
					int32 affected = conn->GetRowCount();
					if (affected == 0)
					{
						conn->Unbind();

						// game uid = it.itemuid() (이름은 itemuid지만 의미는 game_item_uid)
						int64 gameItemUid = (int64)it.itemuid();

						if (!conn->Prepare(L"INSERT INTO ITEMS (owner_id, game_item_uid, template_id, slot_index, count, is_equipped) VALUES (?, ?, ?, ?, ?, ?)"))
						{
							ok = false; break;
						}

						ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbPlayerId, &len);
						ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &gameItemUid, &len);
						ok &= conn->BindParam(3, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
						ok &= conn->BindParam(4, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
						ok &= conn->BindParam(5, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
						ok &= conn->BindParam(6, SQL_C_BIT, SQL_BIT, sizeof(unsigned char), &equipped, &len);

						if (!ok || !conn->Execute())
						{
							ok = false; break;
						}
					}
				}
			}

			// COMMIT / ROLLBACK
			conn->Unbind();
			if (ok) conn->Execute(L"COMMIT TRAN");
			else    conn->Execute(L"ROLLBACK TRAN");

			resPkt.set_success(ok);

			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);
		}));

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_ITEM_CREATE(PacketSessionRef& session, Protocol::S2S_REQ_ITEM_CREATE& pkt)
{
	DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_ITEM_CREATE);

	
	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_GAME_ITEM_UID_SEED(PacketSessionRef& session, Protocol::S2S_REQ_GAME_ITEM_UID_SEED& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_GAME_ITEM_UID_SEED);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (!conn) return;

			Protocol::S2S_RES_GAME_ITEM_UID_SEED res;
			res.set_success(false);
			res.set_next_uid(0);

			conn->Unbind();

			int64 nextUid = 0;
			SQLLEN len = 0;

			// 핵심 쿼리
			// (ITEMS 비어있으면 1000000부터 시작하게 999999 + 1)
			if (conn->Prepare(L"SELECT ISNULL(MAX(game_item_uid), 999999) + 1 AS next_uid FROM ITEMS"))
			{
				conn->BindCol(1, SQL_C_SBIGINT, sizeof(int64), &nextUid, &len);

				if (conn->Execute() && conn->Fetch())
				{
					res.set_success(true);
					res.set_next_uid((uint64)nextUid);
				}
			}

			GDBConnectionPool->Push(conn);

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(res);
			gameSession->Send(sendBuffer);
		}));

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_QUICKSLOT_LOAD(PacketSessionRef& session, Protocol::S2S_REQ_QUICKSLOT_LOAD& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushHighJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_QUICKSLOT_LOAD);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (!conn) return;

			Protocol::S2S_RES_QUICKSLOT_LOAD res;
			res.set_success(false);
			res.set_playerid(pkt.playerid());
			res.set_gamesessionid(pkt.gamesessionid());

			conn->Unbind();

			int64 dbOwnerId = (int64)pkt.playerid();
			int32 outSlotIndex = 0;
			int32 outRefType = 0;
			int64 outRefId = 0;

			SQLLEN len = 0;

			if (conn->Prepare(L"SELECT slot_index, ref_type, ref_id FROM PLAYER_QUICKSLOT WHERE owner_id = ? ORDER BY slot_index ASC"))
			{
				conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbOwnerId, &len);

				conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &outSlotIndex, &len);
				conn->BindCol(2, SQL_C_SLONG, sizeof(int32), &outRefType, &len);
				conn->BindCol(3, SQL_C_SBIGINT, sizeof(int64), &outRefId, &len);

				if (conn->Execute())
				{
					res.set_success(true);

					while (conn->Fetch())
					{
						if (outSlotIndex < 0 || outSlotIndex >= QS_MAX)
							continue;

						auto* s = res.add_slots();
						s->set_slotindex(outSlotIndex);
						s->set_reftype(static_cast<Protocol::QuickSlotRefType>(outRefType));
						s->set_refid((uint64)outRefId);
					}

					std::cout << " [DB] Loaded QuickSlots for Player: " << pkt.playerid()
						<< " Count: " << res.slots_size() << std::endl;
				}
			}

			GDBConnectionPool->Push(conn);

			auto sb = DBAgentPacketHandler::MakeSendBuffer(res);
			gameSession->Send(sb);
		}));

	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_SAVE_QUICKSLOT(PacketSessionRef& session, Protocol::S2S_REQ_SAVE_QUICKSLOT& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_SAVE_QUICKSLOT);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (!conn) return;

			Protocol::S2S_RES_SAVE_QUICKSLOT res;
			res.set_success(false);
			res.set_playerid(pkt.playerid());

			bool ok = true;
			int64 dbOwnerId = (int64)pkt.playerid();
			SQLLEN len = 0;

			// BEGIN
			conn->Unbind();
			ok = conn->Execute(L"BEGIN TRAN");

			// 1) DELETE all
			if (ok)
			{
				conn->Unbind();
				if (!conn->Prepare(L"DELETE FROM PLAYER_QUICKSLOT WHERE owner_id = ?"))
					ok = false;
				else
				{
					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbOwnerId, &len);
					ok &= conn->Execute();
				}
			}

			// 2) INSERT snapshot (non-empty only)
			if (ok)
			{
				for (int i = 0; i < pkt.slots_size(); ++i)
				{
					const auto& s = pkt.slots(i);

					// 빈 슬롯은 DB에 저장 안 함
					if (s.reftype() == Protocol::QS_NONE || s.refid() == 0)
						continue;

					conn->Unbind();

					int32 slotIndex = s.slotindex();

					if (slotIndex < 0 || slotIndex >= QS_MAX)
						continue;

					int32 refType = (int32)s.reftype();
					int64 refId = (int64)s.refid();

					if (!conn->Prepare(L"INSERT INTO PLAYER_QUICKSLOT (owner_id, slot_index, ref_type, ref_id) VALUES (?, ?, ?, ?)"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &dbOwnerId, &len);
					ok &= conn->BindParam(2, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(3, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &refType, &len);
					ok &= conn->BindParam(4, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &refId, &len);

					if (!ok || !conn->Execute())
					{
						ok = false; break;
					}
				}
			}

			// COMMIT/ROLLBACK
			conn->Unbind();
			if (ok) conn->Execute(L"COMMIT TRAN");
			else    conn->Execute(L"ROLLBACK TRAN");

			res.set_success(ok);

			GDBConnectionPool->Push(conn);

			auto sb = DBAgentPacketHandler::MakeSendBuffer(res);
			gameSession->Send(sb);
		}));

	return true;
}

// [Game -> DB] Trade atomic commit (Phase 2)
// Apply both players' inventory snapshots in a single SQL transaction.
bool DBAgentPacketHandler::Handle_S2S_REQ_TRADE_COMMIT(
	PacketSessionRef& session,
	Protocol::S2S_REQ_TRADE_COMMIT& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			DBAgentMetrics::ScopedRequestMetrics requestScope(DBAgentPacketHandler::PKT_S2S_REQ_TRADE_COMMIT);

			DBConnection* conn = GDBConnectionPool->Pop();
			if (!conn)
				return;

			bool ok = true;
			SQLLEN len = 0;

			int64 playerAId = (int64)pkt.playeraid();
			int64 playerBId = (int64)pkt.playerbid();
			int64 finalGoldA = (int64)pkt.finalgolda();
			int64 finalGoldB = (int64)pkt.finalgoldb();

			Protocol::S2S_RES_TRADE_COMMIT res;
			res.set_tradeid(pkt.tradeid());
			res.set_channelid(pkt.channelid());
			res.set_mapid(pkt.mapid());
			res.set_instanceid(pkt.instanceid());
			res.set_requestid(pkt.requestid());
			res.set_success(false);
			res.set_failcode(Protocol::TRADE_FAIL_INTERNAL);

			// ===============================
			// BEGIN TRAN
			// ===============================
			conn->Unbind();
			ok = conn->Execute(L"BEGIN TRAN");

			// ===============================
			// A: DELETE
			// ===============================
			if (ok)
			{
				for (uint64 uid : pkt.deletedaitemuids())
				{
					int64 itemUid = (int64)uid;

					conn->Unbind();
					if (!conn->Prepare(L"DELETE FROM ITEMS WHERE owner_id=? AND game_item_uid=?"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerAId, &len);
					ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);
					ok &= conn->Execute();

					if (!ok) break;
				}
			}

			// ===============================
			// A: UPSERT
			// ===============================
			if (ok)
			{
				for (const auto& item : pkt.finalaitems())
				{
					int64 itemUid = (int64)item.itemuid();
					int32 templateId = item.templateid();
					int32 slotIndex = item.slot();
					int32 count = item.count();
					int32 isEquipped = item.isequipped();

					conn->Unbind();
					if (!conn->Prepare(
						L"UPDATE ITEMS SET owner_id=?, template_id=?, slot_index=?, count=?, is_equipped=? "
						L"WHERE game_item_uid=?; "
						L"IF @@ROWCOUNT = 0 "
						L"INSERT INTO ITEMS(owner_id, game_item_uid, template_id, slot_index, count, is_equipped) "
						L"VALUES(?,?,?,?,?,?)"))
					{
						ok = false; break;
					}

					// UPDATE
					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerAId, &len);
					ok &= conn->BindParam(2, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
					ok &= conn->BindParam(3, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(4, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
					ok &= conn->BindParam(5, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &isEquipped, &len);
					ok &= conn->BindParam(6, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);

					// INSERT
					ok &= conn->BindParam(7, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerAId, &len);
					ok &= conn->BindParam(8, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);
					ok &= conn->BindParam(9, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
					ok &= conn->BindParam(10, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(11, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
					ok &= conn->BindParam(12, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &isEquipped, &len);

					ok &= conn->Execute();

					if (!ok) break;
				}
			}

			// ===============================
			// B: DELETE + UPSERT (동일 패턴)
			// ===============================
			if (ok)
			{
				for (uint64 uid : pkt.deletedbitemuids())
				{
					int64 itemUid = (int64)uid;

					conn->Unbind();
					if (!conn->Prepare(L"DELETE FROM ITEMS WHERE owner_id=? AND game_item_uid=?"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerBId, &len);
					ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);
					ok &= conn->Execute();

					if (!ok) break;
				}
			}

			if (ok)
			{
				for (const auto& item : pkt.finalbitems())
				{
					int64 itemUid = (int64)item.itemuid();
					int32 templateId = item.templateid();
					int32 slotIndex = item.slot();
					int32 count = item.count();
					int32 isEquipped = item.isequipped();

					conn->Unbind();
					if (!conn->Prepare(
						L"UPDATE ITEMS SET owner_id=?, template_id=?, slot_index=?, count=?, is_equipped=? "
						L"WHERE game_item_uid=?; "
						L"IF @@ROWCOUNT = 0 "
						L"INSERT INTO ITEMS(owner_id, game_item_uid, template_id, slot_index, count, is_equipped) "
						L"VALUES(?,?,?,?,?,?)"))
					{
						ok = false; break;
					}

					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerBId, &len);
					ok &= conn->BindParam(2, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
					ok &= conn->BindParam(3, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(4, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
					ok &= conn->BindParam(5, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &isEquipped, &len);
					ok &= conn->BindParam(6, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);

					ok &= conn->BindParam(7, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerBId, &len);
					ok &= conn->BindParam(8, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &itemUid, &len);
					ok &= conn->BindParam(9, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &templateId, &len);
					ok &= conn->BindParam(10, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &slotIndex, &len);
					ok &= conn->BindParam(11, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &count, &len);
					ok &= conn->BindParam(12, SQL_C_SLONG, SQL_INTEGER, sizeof(int32), &isEquipped, &len);

					ok &= conn->Execute();

					if (!ok) break;
				}
			}

			// ===============================
			// COMMIT / ROLLBACK
			// ===============================
			if (ok)
			{
				conn->Unbind();
				if (!conn->Prepare(L"UPDATE PLAYERS SET gold=? WHERE playerId=?"))
				{
					ok = false;
				}
				else
				{
					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &finalGoldA, &len);
					ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerAId, &len);
					ok &= conn->Execute();
				}
			}

			if (ok)
			{
				conn->Unbind();
				if (!conn->Prepare(L"UPDATE PLAYERS SET gold=? WHERE playerId=?"))
				{
					ok = false;
				}
				else
				{
					ok &= conn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &finalGoldB, &len);
					ok &= conn->BindParam(2, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64), &playerBId, &len);
					ok &= conn->Execute();
				}
			}

			conn->Unbind();
			if (ok) conn->Execute(L"COMMIT TRAN");
			else    conn->Execute(L"ROLLBACK TRAN");

			res.set_success(ok);
			res.set_failcode(ok ? Protocol::TRADE_FAIL_NONE : Protocol::TRADE_FAIL_INTERNAL);

			GDBConnectionPool->Push(conn);

			gameSession->Send(DBAgentPacketHandler::MakeSendBuffer(res));
		}));

	return true;
}
