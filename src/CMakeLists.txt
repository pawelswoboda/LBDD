add_library(bdd_node bdd_node.cpp)
target_link_libraries(bdd_node bdd)

add_library(bdd_node_cache bdd_node_cache.cpp) 
target_link_libraries(bdd_node_cache bdd_node bdd)

add_library(bdd_node_depth bdd_node_depth.cpp)
target_link_libraries(bdd_node_depth bdd_node bdd)

add_library(bdd_var bdd_var.cpp)
target_link_libraries(bdd_var bdd_node_cache bdd)

add_library(bdd_memo_cache bdd_memo_cache.cpp)
target_link_libraries(bdd_memo_cache bdd_node bdd_node_cache bdd)

add_library(bdd_mgr bdd_mgr.cpp)
target_link_libraries(bdd_mgr bdd_node_cache bdd_var bdd_memo_cache bdd)

add_library(bdd_collection bdd_collection.cpp)
target_link_libraries(bdd_collection bdd_node_cache bdd_var bdd_memo_cache bdd_mgr bdd)

target_link_libraries(bdd INTERFACE bdd_node)
target_link_libraries(bdd INTERFACE bdd_node_cache)
target_link_libraries(bdd INTERFACE bdd_var)
target_link_libraries(bdd INTERFACE bdd_memo_cache)
target_link_libraries(bdd INTERFACE bdd_mgr)
target_link_libraries(bdd INTERFACE bdd_collection)
