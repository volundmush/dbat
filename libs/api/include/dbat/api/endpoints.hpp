#pragma once
#include "channel.hpp"

namespace dbat::api {
    
    std::shared_ptr<volcano::web::Router> init_router();
}