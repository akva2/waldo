/*
 *  Copyright (C) 2024 SINTEF Digital
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  See LICENSE.md for more information.
 */

#include <string_view>
#include <vector>

namespace BVH { struct Triangle; }

namespace STLReader {
    //! \brief Read a STL file.
    //! \param[in] path Path to file or buffer to read
    //! \param[in] is_file True if path is a filename
    std::vector<BVH::Triangle> read(std::string_view path,
                                    bool is_file = true);
};
