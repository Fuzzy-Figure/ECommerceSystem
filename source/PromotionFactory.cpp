#include "../header/PromotionFactory.h"
#include <algorithm>
#include <unordered_map>
#include <functional>

namespace {
    // 把 JSON 数值稳健转 double（字符串形式也接受）
    double toDouble(const nlohmann::json& j, const char* key, double def) {
        if (!j.contains(key)) return def;
        const auto& v = j[key];
        if (v.is_number()) return v.get<double>();
        if (v.is_string()) { try { return std::stod(v.get<std::string>()); } catch (...) {} }
        return def;
    }
    int toInt(const nlohmann::json& j, const char* key, int def) {
        if (!j.contains(key)) return def;
        const auto& v = j[key];
        if (v.is_number()) return v.get<int>();
        if (v.is_string()) { try { return std::stoi(v.get<std::string>()); } catch (...) {} }
        return def;
    }

    // 固定顺序优先级：数字越小越内层（越先应用）
    int typePriority(const std::string& type) {
        if (type == "reduction")  return 1;
        if (type == "discount")   return 2;
        if (type == "tiered")     return 3;
        if (type == "freeitem")   return 4;
        if (type == "coupon")     return 5;
        return 999;  // 未知类型排到最后
    }

    // 解析阶梯折扣参数：{"tiers":[[2,0.9],[3,0.8]]}
    std::vector<std::pair<int, double>> parseTiers(const nlohmann::json& params) {
        std::vector<std::pair<int, double>> tiers;
        if (!params.contains("tiers") || !params["tiers"].is_array()) return tiers;
        for (const auto& t : params["tiers"]) {
            if (!t.is_array() || t.size() < 2) continue;
            const int n = toInt(t, 0, 0);
            double r = 0.0;
            const auto& rv = t[1];
            if (rv.is_number()) r = rv.get<double>();
            else if (rv.is_string()) { try { r = std::stod(rv.get<std::string>()); } catch (...) {} }
            if (n > 0 && r > 0.0 && r <= 1.0) tiers.emplace_back(n, r);
        }
        return tiers;
    }
}

std::unique_ptr<Promotion>
PromotionFactory::createSingle(const std::string& type, const nlohmann::json& params) {
    if (type == "discount") {
        return std::make_unique<Discount>(toDouble(params, "rate", 1.0));
    }
    if (type == "tiered") {
        return std::make_unique<TieredDiscount>(parseTiers(params));
    }
    if (type == "freeitem") {
        return std::make_unique<FreeItem>(toInt(params, "buyN", 0), toInt(params, "freeM", 0));
    }
    if (type == "reduction") {
        return std::make_unique<Reduction>(toDouble(params, "threshold", 0.0),
                                           toDouble(params, "reduce",    0.0));
    }
    if (type == "coupon") {
        return std::make_unique<Coupon>(toDouble(params, "amount", 0.0));
    }
    return nullptr;  // 未知类型
}

std::unique_ptr<Promotion>
PromotionFactory::buildChain(const std::vector<PromotionDAO::Config>& configs) {
    if (configs.empty()) return nullptr;

    // 复制并按固定优先级排序（数字越小越内层）
    auto sorted = configs;
    std::sort(sorted.begin(), sorted.end(),
              [](const PromotionDAO::Config& a, const PromotionDAO::Config& b) {
                  return typePriority(a.type) < typePriority(b.type);
              });

    // 链式包装：第一个是最内层，setInner 后续每一个
    std::unique_ptr<Promotion> head = nullptr;
    for (const auto& cfg : sorted) {
        auto p = createSingle(cfg.type, cfg.params);
        if (!p) continue;  // 跳过未知类型
        if (!head) {
            head = std::move(p);
        } else {
            p->setInner(std::move(head));
            head = std::move(p);
        }
    }
    return head;
}

std::unique_ptr<Promotion> PromotionFactory::buildChainFromDB(PromotionDAO& dao) {
    return buildChain(dao.findEnabled());
}
