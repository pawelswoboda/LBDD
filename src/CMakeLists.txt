add_library(bdd_node bdd_node.cpp)

add_library(bdd_node_cache bdd_node_cache.cpp) 
target_link_libraries(bdd_node_cache bdd_node)

add_library(bdd_node_depth bdd_node_depth.cpp)
target_link_libraries(bdd_node_depth bdd_node)

add_library(bdd_var bdd_var.cpp)
target_link_libraries(bdd_var bdd_node_cache)

add_library(bdd_unique_table bdd_unique_table.cpp)
target_link_libraries(bdd_unique_table bdd_var)

add_library(bdd_memo_cache bdd_memo_cache.cpp)
target_link_libraries(bdd_memo_cache bdd_node bdd_node_cache)

add_library(bdd_mgr bdd_mgr.cpp)
target_link_libraries(bdd_mgr bdd_node_cache bdd_unique_table bdd_memo_cache)

add_library(bdd INTERFACE)
target_link_libraries(bdd INTERFACE bdd_node)
target_link_libraries(bdd INTERFACE bdd_node_cache)
target_link_libraries(bdd INTERFACE bdd_var)
target_link_libraries(bdd INTERFACE bdd_unique_table)
target_link_libraries(bdd INTERFACE bdd_memo_cache)
target_link_libraries(bdd INTERFACE bdd_mgr)
