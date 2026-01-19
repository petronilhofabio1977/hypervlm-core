#pragma once
#include <cstdint>
#include <cstdlib>

namespace hvlm::dre_config {

struct Config {
    double w_relevance = 1.0;
    double w_cost = 1.0;
    double w_physics = 1.0;
    double w_priority = 1.0;
    double w_morton = 0.1;
    double skip_threshold = 50.0; // if skip_hint < this, node is considered skippable
    bool adaptive = true;         // enable adaptive weight adjustment
    int  log_level = 1;           // 0 none, 1 basic, 2 debug, 3 trace
};

inline Config& global() {
    static Config c;
    // Allow runtime overrides from env vars:
    const char* e;
    if ((e = std::getenv("DRE_W_RELEVANCE"))) c.w_relevance = atof(e);
    if ((e = std::getenv("DRE_W_COST")))      c.w_cost = atof(e);
    if ((e = std::getenv("DRE_W_PHYSICS")))   c.w_physics = atof(e);
    if ((e = std::getenv("DRE_W_PRIORITY")))  c.w_priority = atof(e);
    if ((e = std::getenv("DRE_LOG")))         c.log_level = atoi(e);
    if ((e = std::getenv("DRE_ADAPTIVE")))    c.adaptive = (atoi(e) != 0);
    return c;
}

} // namespace hvlm::dre_config
