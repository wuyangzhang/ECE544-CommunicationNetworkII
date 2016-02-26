// Parameters:
// my_GUID
// GNRS_server_ip
// GNRS_server_port
// GNRS_listen_ip
// GNRS_listen_port
// delay
// period

resp_cache::GNRS_RespCache();
gnrs_rrh::GNRS_ReqRespHandler(NET_ID "NA", MY_GUID $my_GUID, RESP_LISTEN_IP $GNRS_listen_ip, RESP_LISTEN_PORT $GNRS_listen_port, RESP_CACHE resp_cache);
GNRS_ReqGen(DELAY $delay, PERIOD $period)  -> gnrs_req_q::Queue -> Unqueue -> Print(GNRS-req-int) -> [0]gnrs_rrh;
gnrs_rrh[0] -> Print(GNRS-req-ext, 100) -> Socket(UDP, $GNRS_server_ip, $GNRS_server_port);
Socket(UDP, 0.0.0.0, $GNRS_listen_port) -> gnrs_resp_q::Queue -> Unqueue -> Print(GNRS-resp, 100) -> [1]gnrs_rrh;
gnrs_rrh[1] -> Print(GNRS-lookup-resp) -> Discard;
