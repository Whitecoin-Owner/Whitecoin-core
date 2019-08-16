#!/bin/bash
container_id="uvm_uvmbuild_dev"
docker exec $container_id ./build_deps.sh
docker exec $container_id cmake .
docker exec $container_id make
docker exec $container_id chmod +x /code/uvm_single_exec
docker exec $container_id chmod +x /code/simplechain_runner
docker exec $container_id ./test/install_deps.sh
docker exec $container_id ./test/run_tests.sh
test -e ./uvm_single_exec
test -e ./simplechain_runner
