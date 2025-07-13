#include "mongoose.h"
#include "rest_storage.h"

#include <iostream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

FederatedStorage storage;

static void send_json(struct mg_connection* c, const std::string& data) {
    mg_printf(c,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
        "Content-Length: %lu\r\n\r\n%s",
        data.length(), data.c_str());
}

static void send_error(struct mg_connection* c, int code, const std::string& message) {
    std::string payload = "{ \"error\": \"" + message + "\" }";
    mg_printf(c,
        "HTTP/1.1 %d ERROR\r\nContent-Type: application/json\r\n"
        "Content-Length: %lu\r\n\r\n%s",
        code, payload.length(), payload.c_str());
}

static std::string get_query_param(struct mg_str* query_string, const std::string& key) {
    char buf[100];
    mg_get_http_var(query_string, key.c_str(), buf, sizeof(buf));
    return std::string(buf);
}

static void handle_request(struct mg_connection* c, int ev, void* ev_data) {
    struct http_message* hm = (struct http_message*) ev_data;

    std::string uri(hm->uri.p, hm->uri.len);
    std::string method(hm->method.p, hm->method.len);
    std::string body(hm->body.p, hm->body.len);

    std::cout << "📥 " << method << " " << uri << " (body: " << hm->body.len << " bytes)" << std::endl;

    try {
        // Defensive JSON parsing block
        auto parse_json_or_fail = [&](json& payload_out) -> bool {
            if (body.empty()) {
                send_error(c, 400, "Empty request body");
                return false;
            }
            try {
                payload_out = json::parse(body);
                return true;
            } catch (const std::exception& e) {
                send_error(c, 400, std::string("Invalid JSON: ") + e.what());
                return false;
            }
        };

        if (uri == "/input" && method == "POST") {
            json payload;
            if (!parse_json_or_fail(payload)) return;
            int round = payload["round"];
            storage.StoreInput(round, payload);
            send_json(c, R"({"status":"input received"})");

        } else if (uri == "/input" && method == "GET") {
            int round = std::stoi(get_query_param(&hm->query_string, "round"));
            json input = storage.GetInput(round);
            send_json(c, input.dump());

        } else if (uri == "/encrypted-data/ciphertext" && method == "POST") {
            json payload;
            if (!parse_json_or_fail(payload)) return;
            CiphertextEntry entry {
                payload["client_id"],
                payload["ciphertext"],
                payload["public_key"],
                payload["eval_mult_key"],
                payload["eval_sum_key"]
            };
            int round = payload["round"];
            storage.AddCiphertext(round, entry);
            send_json(c, R"({"status":"ciphertext received"})");

        } else if (uri == "/encrypted-data/rekey" && method == "POST") {
            json payload;
            if (!parse_json_or_fail(payload)) return;
            ReKeyEntry entry {
                payload["rekey_from"],
                payload["rekey_to"],
                payload["rekey"]
            };
            int round = payload["round"];
            storage.AddReKey(round, entry);
            send_json(c, R"({"status":"rekey received"})");

        } else if (uri == "/encrypted-data" && method == "GET") {
            int round = std::stoi(get_query_param(&hm->query_string, "round"));
            json result;
            result["ciphertexts"] = json::array();
            for (const auto& ct : storage.GetCiphertexts(round)) {
                result["ciphertexts"].push_back({
                    {"client_id", ct.client_id},
                    {"ciphertext", ct.ciphertext_b64},
                    {"public_key", ct.public_key_b64},
                    {"eval_mult_key", ct.eval_mult_key_b64},
                    {"eval_sum_key", ct.eval_sum_key_b64}
                });
            }
            result["rekeys"] = json::array();
            for (const auto& rk : storage.GetRekeys(round)) {
                result["rekeys"].push_back({
                    {"rekey_from", rk.rekey_from},
                    {"rekey_to", rk.rekey_to},
                    {"rekey", rk.rekey_b64}
                });
            }
            send_json(c, result.dump());

        } else if (uri == "/result" && method == "POST") {
            json payload;
            if (!parse_json_or_fail(payload)) return;
            int round = payload["round"];
            std::string client_id = payload["client_id"];
            storage.StoreResult(round, client_id, payload);

            std::string logFile = "logs/round_" + std::to_string(round) + "_output.json";
            storage.LogRoundToFile(round, logFile);
            send_json(c, R"({"status":"result stored and logged"})");

        } else if (uri == "/result" && method == "GET") {
            int round = std::stoi(get_query_param(&hm->query_string, "round"));
            std::string client_id = get_query_param(&hm->query_string, "client_id");

            json full_result = storage.GetResult(round, client_id);
            if (!full_result.contains("client_id") || full_result["client_id"] != client_id) {
                send_error(c, 404, "No result found for this client and round");
                return;
            }

            send_json(c, full_result.dump());
        }

        else {
            send_error(c, 404, "Unknown endpoint");
        }

    } catch (const std::exception& e) {
        send_error(c, 500, std::string("Exception: ") + e.what());
    } catch (...) {
        send_error(c, 500, "Unhandled error");
    }
}

int main() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, nullptr);

    struct mg_connection* c = mg_bind(&mgr, "0.0.0.0:8000", [](mg_connection* c, int ev, void* ev_data) {
        if (ev == MG_EV_HTTP_REQUEST) {
            handle_request(c, ev, ev_data);
        }
    });

    if (c == nullptr) {
        std::cerr << "Failed to bind to port 8000" << std::endl;
        return 1;
    }

    mg_set_protocol_http_websocket(c);

    std::cout << "[REST Server] Listening on http://localhost:8000" << std::endl;

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    return 0;
}
