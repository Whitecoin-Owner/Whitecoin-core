#!/bin/bash
container_id="uvm_uvmbuild_dev"
docker exec $container_id ./test/run_tests.sh
