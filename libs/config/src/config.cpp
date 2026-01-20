#include "config/config.hpp"
#include "net/net.hpp"
#include "jwt/jwt.hpp"
#include "dotenv/dotenv.hpp"
#include "logging/Log.hpp"

namespace dbat::config {
    void init(std::string_view log_file) {
        auto log_options = dbat::log::Options();
        log_options.file_path = "logs/" + std::string(log_file) + ".log";
        dbat::log::init(log_options);
        dbat::dotenv::load_env_file(".env", false);
        dbat::dotenv::load_env_file(".env.local", true);
    
        auto get_env = [](const char* key) -> const char* {
            const char* value = std::getenv(key);
            return (value && *value) ? value : nullptr;
        };

        if (const char* host = get_env("HTTP_HOST")) {
            auto addr = dbat::net::parse_address(host);
            if (addr) {
                dbat::net::tcp_config.tcp_address = *addr;
            } else {
                LERROR("Invalid HTTP_HOST: %s", host);
            }
        }

        if (const char* port = get_env("HTTP_PORT")) {
            char* end = nullptr;
            const long value = std::strtol(port, &end, 10);
            if (end != port && value > 0 && value <= 65535) {
                dbat::net::tcp_config.tcp_port = static_cast<uint16_t>(value);
            } else {
                LERROR("Invalid HTTP_PORT: %s", port);
            }
        }

        if (const char* host = get_env("HTTPS_HOST")) {
            auto addr = dbat::net::parse_address(host);
            if (addr) {
                dbat::net::tls_config.address = *addr;
            } else {
                LERROR("Invalid HTTPS_HOST: %s", host);
            }
        }

        if (const char* port = get_env("HTTPS_PORT")) {
            char* end = nullptr;
            const long value = std::strtol(port, &end, 10);
            if (end != port && value > 0 && value <= 65535) {
                dbat::net::tls_config.port = static_cast<uint16_t>(value);
            } else {
                LERROR("Invalid HTTPS_PORT: %s", port);
            }
        }

        if (const char* cert = get_env("TLS_ANSWER_FILE")) {
            dbat::net::tls_config.cert_path = cert;
        } else if (const char* cert = get_env("TLS_CERT_FILE")) {
            dbat::net::tls_config.cert_path = cert;
        }

        if (const char* key = get_env("TLS_KEY_FILE")) {
            dbat::net::tls_config.key_path = key;
        }

        if (!dbat::net::tls_config.cert_path.empty() || !dbat::net::tls_config.key_path.empty()) {
            auto ctx = dbat::net::create_ssl_context(dbat::net::tls_config.cert_path, dbat::net::tls_config.key_path);
                if (ctx) {
                    dbat::net::tls_config.ssl_context = *ctx;
                    LINFO("TLS enabled using cert {}", dbat::net::tls_config.cert_path.string());
                } else {
                    LERROR("Failed to initialize TLS context: {}", ctx.error());
                }
        }

        if (const char* secret = get_env("JWT_SECRET")) {
            dbat::jwt::jwt_config.secret = secret;
        }

        if (const char* exp = get_env("JWT_EXPIRE_MINUTES")) {
            char* end = nullptr;
            const long value = std::strtol(exp, &end, 10);
            if (end != exp && value > 0) {
                dbat::jwt::jwt_config.token_expiry = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(value));
            } else {
                LERROR("Invalid JWT_EXPIRE_MINUTES: %s", exp);
            }
        }

        if (const char* exp = get_env("JWT_REFRESH_EXPIRE_MINUTES")) {
            char* end = nullptr;
            const long value = std::strtol(exp, &end, 10);
            if (end != exp && value > 0) {
                dbat::jwt::jwt_config.refresh_token_expiry = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(value));
            } else {
                LERROR("Invalid JWT_REFRESH_EXPIRE_MINUTES: %s", exp);
            }
        }

        if (const char* iss = get_env("JWT_ISSUER")) {
            dbat::jwt::jwt_config.issuer = iss;
        }

        if (const char* aud = get_env("JWT_AUDIENCE")) {
            dbat::jwt::jwt_config.audience = aud;
        }
    
    }
}
