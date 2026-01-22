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

bool ClientPacketHandler::Handle_C_DUNGEON_ENTER_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_ENTER_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const int32 dungeonMapId = pkt.dungeonmapid();

	DataManager* dm = DataManager::Instance();
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

	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

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

			//  channelId는 "player 값"으로만 읽는다 (Room actor에서)
			gr->PushJob([gr, requesterId, dungeonMapId, spawn]()
				{
					int32 channelId = 0;

					// 네 GameRoom에 이 함수가 이미 있다고 했던 경계 규칙 기반
					// (없으면: FindPlayer_ActorOnly를 네가 쓰는 이름으로 바꿔)
					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId);
					if (p)
						channelId = p->GetChannelId();

					PartyActor::Instance().Push([requesterId, channelId, dungeonMapId, spawn]()
						{
							auto& core = PartyActor::Instance().Core();
							const uint64 partyId = core.GetPartyIdByPlayerId(requesterId);

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

							// 이미 던전 메타 있으면 막기
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

							InstanceActor::Instance().Push([partyId, members, channelId, dungeonMapId, spawn, requesterId]()
								{
									InstanceManagerCore::InstanceInfo inst;
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

									PartyActor::Instance().Push([partyId, instanceId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.SetPartyInstance(partyId, instanceId, PartyManagerCore::DungeonState::IN_DUNGEON);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::IN_DUNGEON);
										});

									// requester에게 결과
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

									// 파티원 전원 MapChangeBegin
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

												// return 저장은 Room actor에서 (player 값 기반)
												gr2->PushJob([gr2, self2, pid, dungeonMapId, instanceId, spawn]() mutable
													{
														gr2->SaveReturnLocation_ActorOnly(pid);

														//  채널은 Room actor에서 Player 값으로만
														int32 targetChannelId = 1;
														if (auto p = gr2->FindPlayer_ActorOnly(pid))
														{
															targetChannelId = p->GetChannelId();
															if (targetChannelId <= 0) targetChannelId = 1;
														}

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

bool ClientPacketHandler::Handle_C_DUNGEON_EXIT_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_EXIT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	// SessionActor에서 시작 (player 접근 금지)
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

			// 던전 여부 판정은 RoomActor에서
			gr->PushJob([gr, requesterId]()
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId); // <- 네 함수명 맞춰
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

					// 0이면 던전 아님
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

					// ===== 여기부터는 PartyActor/InstanceActor 흐름 =====
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

							// EXITING transition
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
									const bool closedOk = InstanceActor::Instance().Core().CloseForParty(partyId, closed);

									if (!closedOk)
									{
										// 롤백: 다시 IN_DUNGEON
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

									// room closing 마킹(purge 유도)
									if (closed.instanceId != 0 && GRoomManager)
									{
										auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
										if (room) room->MarkClosing(true);
									}

									// party 메타 정리 + transition 종료(NONE)
									PartyActor::Instance().Push([partyId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.ClearPartyInstance(partyId);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::NONE);
										});

									// requester에게 exit res(성공) - return은 RoomActor에서 계산
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

									// 파티원 전원 월드 복귀 MapChangeBegin (온라인만)
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
