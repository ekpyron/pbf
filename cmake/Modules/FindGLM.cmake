# Find glm
#
# This module defines
# GLM_FOUND - whether glm was found
# GLM_INCLUDE_DIR - the location of glm
#

find_path (GLM_INCLUDE_DIR glm/glm.hpp)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GLM DEFAULT_MSG GLM_INCLUDE_DIR)
mark_as_advanced (GLM_INCLUDE_DIR)
