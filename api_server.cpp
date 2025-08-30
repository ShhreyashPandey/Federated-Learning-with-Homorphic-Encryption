#include "mongoose.h"
#include "rest_storage.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Global storage instance
FederatedStorage storage;

static void send_json(struct mg_connection* c, const std::string& data) {
    mg_printf(c,
              "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
              "Content-Length: %lu\r\n\r\n%s",
              (unsigned long)data.size(), data.c_str());
}

static void send_error(struct mg_connection* c, int code, const std::string& message) {
    std::string payload = "{ \"error\": \"" + message + "\" }";
    mg_printf(c,
              "HTTP/1.1 %d ERROR\r\nContent-Type: application/json\r\n"
              "Content-Length: %lu\r\n\r\n%s",
              code, (unsigned long)payload.size(), payload.c_str());
}

static std::string get_query_param(struct mg_str* query_string, const std::string& key) {
    char buf[1024] = {0};  // Increased buffer size for larger params if needed
    mg_get_http_var(query_string, key.c_str(), buf, sizeof(buf));
    return std::string(buf);
}

static void handle_request(struct mg_connection* c, int ev, void* ev_data) {
    auto* hm = (struct http_message*)ev_data;

    std::string uri(hm->uri.p, hm->uri.len);
    std::string method(hm->method.p, hm->method.len);
    std::string body(hm->body.p, hm->body.len);

    std::cout << "📥 " << method << " " << uri << " (body length: " << body.length() << " bytes)" << std::endl;

    try {
        json payload;
        if (method == "POST" && !body.empty()) {
            payload = json::parse(body);
        }

        // KEY MANAGEMENT
        if (uri == "/c2s/public_key" && method == "POST") {
            if (!payload.contains("client_id") || !payload.contains("public_key") ||
                !payload.contains("eval_mult_key") || !payload.contains("eval_sum_key")) {
                send_error(c, 400, "Missing required fields in public_key JSON");
                return;
            }

            std::string client_id = payload["client_id"];
            std::string pubkey    = payload["public_key"];
            std::string eval_mult = payload["eval_mult_key"];
            std::string eval_sum  = payload["eval_sum_key"];

            storage.StorePublicKey(client_id, pubkey, eval_mult, eval_sum);
            send_json(c, R"({"status":"public key stored"})");
            return;
        }

        if (uri == "/s2c/public_key" && method == "GET") {
            std::string client_id = get_query_param(&hm->query_string, "client_id");
            if (client_id.empty()) {
                send_error(c, 400, "Missing client_id parameter");
                return;
            }

            auto pk_json = storage.GetPublicKey(client_id);
            if (pk_json.is_null()) {
                send_error(c, 404, "Public key not found");
                return;
            }

            send_json(c, pk_json.dump());
            return;
        }

        // REKEY MANAGEMENT
        if (uri == "/c2s/rekey" && method == "POST") {
            if (!payload.contains("from_client_id") || !payload.contains("to_client_id") || !payload.contains("rekey")) {
                send_error(c, 400, "Missing fields in rekey JSON");
                return;
            }

            std::string from  = payload["from_client_id"];
            std::string to    = payload["to_client_id"];
            std::string rekey = payload["rekey"];

            storage.StoreRekey(from, to, rekey);
            send_json(c, R"({"status":"rekey stored"})");
            return;
        }

        if (uri == "/s2c/rekey" && method == "GET") {
            std::string from = get_query_param(&hm->query_string, "from");
            std::string to   = get_query_param(&hm->query_string, "to");

            if (from.empty() || to.empty()) {
                send_error(c, 400, "Missing 'from' or 'to' parameter");
                return;
            }

            auto rekey_json = storage.GetRekey(from, to);
            if (rekey_json.is_null()) {
                send_error(c, 404, "Rekey not found");
                return;
            }

            send_json(c, rekey_json.dump());
            return;
        }

        // PARAMETERS MANAGEMENT (Encrypted model weights per round per client)
        // Expect JSON payload with "metadata" and "data" keys

        if (uri == "/c2s/params" && method == "POST") {
            if (!payload.contains("metadata") || !payload.contains("data")) {
                send_error(c, 400, "Missing 'metadata' or 'data' in params JSON");
                return;
            }
            json metadata = payload["metadata"];
            json data = payload["data"];

            if (!metadata.contains("client_id") || !metadata.contains("round")) {
                send_error(c, 400, "Missing client_id or round in metadata");
                return;
            }
            if (!data.contains("params")) {
                send_error(c, 400, "Missing params in data");
                return;
            }

            const std::string client = metadata["client_id"];
            int round = metadata["round"];
            const std::string params_b64 = data["params"];

            std::vector<size_t> chunk_counts;
            if (data.contains("chunk_counts")) {
                chunk_counts = data["chunk_counts"].get<std::vector<size_t>>();
            }

            std::vector<size_t> orig_sizes;
            if (data.contains("orig_sizes")) {
                orig_sizes = data["orig_sizes"].get<std::vector<size_t>>();
            }

            // Store entire Base64 encoded ciphertext vector string and chunk counts and original sizes
            storage.StoreParams(client, round, params_b64, chunk_counts, orig_sizes);
            send_json(c, R"({"status":"params stored"})");
            return;
        }

        if (uri == "/s2c/params" && method == "GET") {
            std::string round_str = get_query_param(&hm->query_string, "round");
            if (round_str.empty()) {
                send_error(c, 400, "Missing round parameter");
                return;
            }

            int round = std::stoi(round_str);
            auto params_json = storage.GetAllParams(round);
            if (params_json.empty()) {
                send_error(c, 404, "No client params found for that round");
                return;
            }

            send_json(c, params_json.dump());
            return;
        }

        // AGGREGATED PARAMETERS MANAGEMENT (Base64 encoded serialized vector of ciphertexts per client)
        // Responses formatted with "metadata" and "data" keys

        if (uri == "/c2s/server/agg_params" && method == "POST") {
            if (!payload.contains("round") || !payload.contains("agg_params")) {
                send_error(c, 400, "Missing fields in aggregated params JSON");
                return;
            }

            int round = payload["round"];
            json agg_params_map = payload["agg_params"];  // { client1: base64_vec, client2: base64_vec, ... }

            storage.StoreAggregatedParams(round, agg_params_map);
            send_json(c, R"({"status":"aggregated params stored"})");
            return;
        }

        if (uri == "/s2c/agg_params" && method == "GET") {
            std::string client_id = get_query_param(&hm->query_string, "client_id");
            std::string round_str = get_query_param(&hm->query_string, "round");
            if (client_id.empty() || round_str.empty()) {
                send_error(c, 400, "Missing client_id or round");
                return;
            }

            int round = std::stoi(round_str);
            auto agg = storage.GetAggregatedParam(client_id, round);
            if (agg.is_null() || agg.empty()) {
                send_error(c, 404, "No aggregated param found");
                return;
            }

            auto chunk_counts = storage.GetChunkCounts(client_id, round);
            auto orig_sizes = storage.GetOrigSizes(client_id, round); // << Added retrieval of original sizes

            // Wrap response in "metadata" and "data"
            json response_json = {
                {"metadata", {
                    {"client_id", client_id},
                    {"round", round}
                }},
                {"data", {
                    {"agg_params", agg["agg_params"]}
                }}
            };

            if (!chunk_counts.empty()) {
                response_json["data"]["chunk_counts"] = chunk_counts;
            }
            if (!orig_sizes.empty()) {
                response_json["data"]["orig_sizes"] = orig_sizes; // << Added original sizes in response
            }

            send_json(c, response_json.dump());
            return;
        }

        // RESULTS MANAGEMENT (Accuracy/Metrics)
        if (uri == "/c2s/result" && method == "POST") {
            if (!payload.contains("client_id") || !payload.contains("round") || !payload.contains("accuracy") || !payload.contains("model")) {
                send_error(c, 400, "Missing fields in result JSON");
                return;
            }

            std::string client = payload["client_id"];
            int round = payload["round"];
            double accuracy = payload["accuracy"];
            std::string model = payload["model"];

            storage.StoreResult(client, round, accuracy, model);
            send_json(c, R"({"status":"result stored"})");
            return;
        }

        if (uri == "/s2c/result" && method == "GET") {
            std::string client_id = get_query_param(&hm->query_string, "client_id");
            std::string round_str = get_query_param(&hm->query_string, "round");

            if (client_id.empty() || round_str.empty()) {
                send_error(c, 400, "Missing client_id or round");
                return;
            }

            int round = std::stoi(round_str);
            auto res = storage.GetResult(client_id, round);
            if (res.is_null()) {
                send_error(c, 404, "Result not found");
                return;
            }

            send_json(c, res.dump());
            return;
        }

        send_error(c, 404, "Unknown endpoint");
    }
    catch (const json::exception& je) {
        send_error(c, 400, std::string("JSON parse error: ") + je.what());
    }
    catch (const std::exception& e) {
        send_error(c, 500, std::string("Server exception: ") + e.what());
    }
    catch (...) {
        send_error(c, 500, "Unknown error");
    }
}

int main() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, nullptr);

    struct mg_connection* c = mg_bind(&mgr, "0.0.0.0:8000", [](mg_connection* conn, int ev, void* ev_data) {
        if (ev == MG_EV_HTTP_REQUEST)
            handle_request(conn, ev, ev_data);
    });

    if (!c) {
        std::cerr << "Failed to bind to port 8000\n";
        return 1;
    }

    mg_set_protocol_http_websocket(c);
    std::cout << "[REST Server] Listening on http://localhost:8000\n";

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    return 0;
}

