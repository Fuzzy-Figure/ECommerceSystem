#include "../header/ClientModel.h"

void ClientModel::setProducts(std::vector<Product> products) {
    products_ = std::move(products);
}
