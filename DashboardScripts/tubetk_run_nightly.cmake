##############################################################################
# 
# Library:   TubeTK
# 
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
# 
# All rights reserved. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
##############################################################################

include( ${CTEST_SCRIPT_DIRECTORY}/../../tubetk_config.cmake )

set( RUN_DASHBOARD_MODEL "Nightly" )

ctest_empty_binary_directory( "${SITE_BINARY_DIR}" )

if( SITE_NIGHTLY_BUILD_TEST )
ctest_run_script( 
  "${SITE_SCRIPT_DIR}/tubetk_build_test.cmake" )
endif( SITE_NIGHTLY_BUILD_TEST )

if( SITE_NIGHTLY_STYLE )
  ctest_run_script( 
    "${SITE_SCRIPT_DIR}/tubetk_style.cmake" )
endif( SITE_NIGHTLY_STYLE )
  
if( SITE_NIGHTLY_COVERAGE )
  ctest_run_script( 
    "${SCRIPT_DIR}/tubetk_coverage.cmake" )
endif( SITE_NIGHTLY_COVERAGE )
 
if( SITE_NIGHTLY_MEMORY )
  ctest_run_script( 
    "${SCRIPT_DIR}/tubetk_memory.cmake" )
endif( SITE_NIGHTLY_MEMORY )

set(CTEST_RUN_CURRENT_SCRIPT 0)

