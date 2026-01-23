#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "Player.h"
#include "GameSessionManager.h"
#include "GameRoom.h" 
#include "DataManager.h"
#include "PartyActor.h"
#include "PartyManagerCore.h"
#include "InstanceActor.h"
#include "ClientPacketHandler.MapChangeUtil.h"

// 클라이언트가 던전 입장을 요청했을 때 처리하는 핸들러
// 여기서 파티 상태 확인부터 인스턴스 생성까지 모든 비동기 흐름을 조율한다
bool ClientPacketHandler::Handle_C_DUNGEON_ENTER_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_ENTER_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const int32 dungeonMapId = pkt.dungeonmapid();

	DataManager* dm = DataManager::Instance();
	// 데이터 매니저를 통해 유효한 던전 맵 ID인지 1차 검증
	if (!dm || !dm->IsValidMapId(dungeonMapId) || !dm->IsDungeonMapId(dungeonMapId))
	{
		ps->Post([dungeonMapId](PlayerSessionRef self)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INVALID_MAP);
				self->Send(MakeSendBuffer(res));
			});
		return true;
	}

	const MapConfig* cfg = dm->GetMapConfig(dungeonMapId);
	if (!cfg) return true;

	// 던전 스폰 위치 미리 세팅
	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	// 세션 액터로 넘어가서 안전하게 플레이어 정보를 조회한다
	ps->Post([dungeonMapId, spawn](PlayerSessionRef self) mutable
		{
			const uint64 requesterId = self->GetPlayerId_AnyThread();
			if (requesterId == 0)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			RoomActorRef room = self->GetCurrentRoom_ActorOnly();
			auto gr = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
			if (!gr)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			// 현재 플레이어가 속한 게임 룸 액터로 작업을 넘긴다
			// 여기서 채널 ID 같은 플레이어 데이터를 안전하게 읽는다
			gr->PushJob([gr, requesterId, dungeonMapId, spawn]()
				{
					int32 channelId = 0;

					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId);
					if (p)
						channelId = p->GetChannelId();

					// 파티 매니저 액터에게 던전 생성 가능 여부를 물어본다
					PartyActor::Instance().Push([requesterId, channelId, dungeonMapId, spawn]()
						{
							auto& core = PartyActor::Instance().Core();
							const uint64 partyId = core.GetPartyIdByPlayerId(requesterId);

							// 파티가 없으면 던전 입장 불가
							if (partyId == 0)
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([dungeonMapId](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_ENTER_RES res;
											res.set_success(false);
											res.set_dungeonmapid(dungeonMapId);
											res.set_instanceid(0);
											res.set_reason(Protocol::DUNGEON_ENTER_FAIL_NOT_IN_PARTY);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							// 이미 던전을 돌고 있거나 전송 중인지 확인해서 중복 입장을 막는다
							{
								int64 curInst = 0; PartyManagerCore::DungeonState st; bool tr = false;
								if (core.GetDungeonInfo(partyId, curInst, st, tr))
								{
									if (curInst != 0 || tr)
									{
										if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											s->Post([dungeonMapId](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_ENTER_RES res;
													res.set_success(false);
													res.set_dungeonmapid(dungeonMapId);
													res.set_instanceid(0);
													res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}
								}
							}

							// 파티 상태를 ENTERING으로 변경하여 트랜잭션을 시작한다
							if (!core.TryBeginDungeonTransition(partyId, PartyManagerCore::DungeonState::ENTERING))
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([dungeonMapId](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_ENTER_RES res;
											res.set_success(false);
											res.set_dungeonmapid(dungeonMapId);
											res.set_instanceid(0);
											res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							Vector<uint64> members;
							core.GetMembers(partyId, members);

							// 인스턴스 액터에게 실제 던전 방 생성을 요청한다
							InstanceActor::Instance().Push([partyId, members, channelId, dungeonMapId, spawn, requesterId]()
								{
									InstanceManagerCore::InstanceInfo inst;
									// 던전 생성에 실패하면 파티 상태를 원복하고 에러를 보낸다
									if (!InstanceActor::Instance().Core().CreateOrGetForParty(partyId, channelId, dungeonMapId, members, inst))
									{
										PartyActor::Instance().Push([partyId]()
											{
												auto& pc = PartyActor::Instance().Core();
												pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::NONE);
												pc.ClearPartyInstance(partyId);
											});

										if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											s->Post([dungeonMapId](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_ENTER_RES res;
													res.set_success(false);
													res.set_dungeonmapid(dungeonMapId);
													res.set_instanceid(0);
													res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}

									const int64 instanceId = inst.instanceId;

									// 생성이 완료되면 파티 정보를 업데이트하고 상태를 IN_DUNGEON으로 확정한다
									PartyActor::Instance().Push([partyId, instanceId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.SetPartyInstance(partyId, instanceId, PartyManagerCore::DungeonState::IN_DUNGEON);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::IN_DUNGEON);
										});

									// 요청자에게 성공 패킷 전송
									if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
									{
										req->Post([dungeonMapId, instanceId](PlayerSessionRef self2)
											{
												Protocol::S_DUNGEON_ENTER_RES res;
												res.set_success(true);
												res.set_dungeonmapid(dungeonMapId);
												res.set_instanceid(instanceId);
												res.set_reason(Protocol::DUNGEON_ENTER_OK);
												self2->Send(MakeSendBuffer(res));
											});
									}

									// 파티원 전원에게 맵 이동 패킷을 보내서 던전으로 이동시킨다
									for (uint64 pid : members)
									{
										auto ms = GameSessionManager::GSessionManager->FindByPlayerId(pid);
										if (!ms) continue;

										ms->Post([pid, dungeonMapId, instanceId, spawn](PlayerSessionRef self2) mutable
											{
												if (self2->IsMapChanging())
													return;

												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;

												if (!gr2)
												{
													const int32 targetChannelId = 1;
													MapChangeUtil::SendMapChangeBegin(self2, pid, targetChannelId, dungeonMapId, instanceId, spawn);
													return;
												}

												// 기존 방에서 나가는 처리와 복귀 위치 저장을 위해 룸 액터로 잡을 보낸다
												gr2->PushJob([gr2, self2, pid, dungeonMapId, instanceId, spawn]() mutable
													{
														gr2->SaveReturnLocation_ActorOnly(pid);

														int32 targetChannelId = 1;
														if (auto p = gr2->FindPlayer_ActorOnly(pid))
														{
															targetChannelId = p->GetChannelId();
															if (targetChannelId <= 0) targetChannelId = 1;
														}

														// 최종적으로 클라에게 맵 이동 시작을 알림
														self2->Post([pid, targetChannelId, dungeonMapId, instanceId, spawn](PlayerSessionRef s) mutable
															{
																MapChangeUtil::SendMapChangeBegin(s, pid, targetChannelId, dungeonMapId, instanceId, spawn);
															});
													});
											});
									}
								});
						});
				});
		});

	return true;
}

// 던전 퇴장 요청 처리 핸들러
// 인스턴스 파괴 및 파티 상태 초기화, 원래 있던 맵으로의 귀환을 처리함
bool ClientPacketHandler::Handle_C_DUNGEON_EXIT_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_EXIT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	// 세션 액터에서 작업을 시작한다
	ps->Post([](PlayerSessionRef self)
		{
			const uint64 requesterId = self->GetPlayerId_AnyThread();
			if (requesterId == 0)
			{
				Protocol::S_DUNGEON_EXIT_RES res;
				res.set_success(false);
				res.set_returnmapid(1);
				res.set_returninstanceid(0);
				res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			RoomActorRef room = self->GetCurrentRoom_ActorOnly();
			auto gr = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
			if (!gr)
			{
				Protocol::S_DUNGEON_EXIT_RES res;
				res.set_success(false);
				res.set_returnmapid(1);
				res.set_returninstanceid(0);
				res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			// 현재 룸 액터에서 플레이어가 실제 던전에 있는지 검증
			gr->PushJob([gr, requesterId]()
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId);
					if (!p)
					{
						if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
						{
							s->Post([](PlayerSessionRef self2)
								{
									Protocol::S_DUNGEON_EXIT_RES res;
									res.set_success(false);
									res.set_returnmapid(1);
									res.set_returninstanceid(0);
									res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
									self2->Send(MakeSendBuffer(res));
								});
						}
						return;
					}

					// 인스턴스 ID가 0이면 던전이 아님
					if (p->GetInstanceId() == 0)
					{
						const int32 curMap = p->GetMapId();
						const int64 curInst = p->GetInstanceId();

						if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
						{
							s->Post([curMap, curInst](PlayerSessionRef self2)
								{
									Protocol::S_DUNGEON_EXIT_RES res;
									res.set_success(false);
									res.set_returnmapid(curMap);
									res.set_returninstanceid(curInst);
									res.set_reason(Protocol::DUNGEON_EXIT_FAIL_NOT_IN_DUNGEON);
									self2->Send(MakeSendBuffer(res));
								});
						}
						return;
					}

					// 파티 및 인스턴스 관리 액터로 흐름을 넘김
					PartyActor::Instance().Push([requesterId]()
						{
							auto& core = PartyActor::Instance().Core();
							const uint64 partyId = core.GetPartyIdByPlayerId(requesterId);

							if (partyId == 0)
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_EXIT_RES res;
											res.set_success(false);
											res.set_returnmapid(1);
											res.set_returninstanceid(0);
											res.set_reason(Protocol::DUNGEON_EXIT_FAIL_NOT_IN_PARTY);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							// 퇴장 트랜잭션 시작 (EXITING 상태)
							if (!core.TryBeginDungeonTransition(partyId, PartyManagerCore::DungeonState::EXITING))
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_EXIT_RES res;
											res.set_success(false);
											res.set_returnmapid(1);
											res.set_returninstanceid(0);
											res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							Vector<uint64> members;
							core.GetMembers(partyId, members);

							InstanceActor::Instance().Push([partyId, members, requesterId]()
								{
									InstanceManagerCore::InstanceInfo closed;
									// 인스턴스를 닫고 관련 정보를 받아옴
									const bool closedOk = InstanceActor::Instance().Core().CloseForParty(partyId, closed);

									if (!closedOk)
									{
										// 실패 시 롤백
										PartyActor::Instance().Push([partyId]()
											{
												auto& pc = PartyActor::Instance().Core();
												pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::IN_DUNGEON);
											});

										if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											req->Post([](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_EXIT_RES res;
													res.set_success(false);
													res.set_returnmapid(1);
													res.set_returninstanceid(0);
													res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}

									// 인스턴스 룸에 닫힘 표시를 해서 정리되도록 유도
									if (closed.instanceId != 0 && GRoomManager)
									{
										auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
										if (room) room->MarkClosing(true);
									}

									// 파티 정보를 초기화하고 상태를 NONE으로 돌려놓음
									PartyActor::Instance().Push([partyId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.ClearPartyInstance(partyId);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::NONE);
										});

									// 요청자에게 먼저 결과를 알림
									if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
									{
										req->Post([requesterId](PlayerSessionRef self2)
											{
												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;

												if (!gr2)
												{
													Protocol::S_DUNGEON_EXIT_RES res;
													res.set_success(true);
													res.set_returnmapid(1);
													res.set_returninstanceid(0);
													res.set_reason(Protocol::DUNGEON_EXIT_OK);
													self2->Send(MakeSendBuffer(res));
													return;
												}

												gr2->PushJob([gr2, self2, requesterId]()
													{
														PlayerRef p = gr2->FindPlayer_ActorOnly(requesterId);
														int32 rm = 1; int64 ri = 0; Protocol::PositionInfo rp;

														if (p)
															MapChangeUtil::MakeSafeReturn(p, rm, ri, rp);

														self2->Post([rm, ri](PlayerSessionRef s) mutable
															{
																Protocol::S_DUNGEON_EXIT_RES res;
																res.set_success(true);
																res.set_returnmapid(rm);
																res.set_returninstanceid(ri);
																res.set_reason(Protocol::DUNGEON_EXIT_OK);
																s->Send(MakeSendBuffer(res));
															});
													});
											});
									}

									// 온라인 상태인 파티원 전원을 원래 월드로 복귀시킴
									for (uint64 pid : members)
									{
										auto ms = GameSessionManager::GSessionManager->FindByPlayerId(pid);
										if (!ms) continue;

										ms->Post([pid](PlayerSessionRef self2) mutable
											{
												if (self2->IsMapChanging())
													return;

												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
												if (!gr2) return;

												// 각 플레이어의 복귀 위치를 계산해서 맵 이동 패킷 전송
												gr2->PushJob([gr2, self2, pid]() mutable
													{
														PlayerRef p = gr2->FindPlayer_ActorOnly(pid);
														if (!p) return;

														int32 rm = 1; int64 ri = 0; Protocol::PositionInfo rp;
														MapChangeUtil::MakeSafeReturn(p, rm, ri, rp);

														int32 targetChannelId = p->GetChannelId();
														if (targetChannelId <= 0) targetChannelId = 1;

														self2->Post([pid, targetChannelId, rm, ri, rp](PlayerSessionRef s) mutable
															{
																MapChangeUtil::SendMapChangeBegin(s, pid, targetChannelId, rm, ri, rp);
															});
													});
											});
									}
								});
						});
				});
		});

	return true;
}