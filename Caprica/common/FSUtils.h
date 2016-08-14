#pragma once

#include <string>

#include <boost/utility/string_ref.hpp>

#include <common/CaselessStringComparer.h>
#include <common/identifier_ref.h>

namespace caprica { namespace FSUtils {

identifier_ref basenameAsRef(const identifier_ref& file);
boost::string_ref extensionAsRef(boost::string_ref file);
boost::string_ref filenameAsRef(boost::string_ref file);
identifier_ref parentPathAsRef(const identifier_ref& file);

std::string canonical(const std::string& path);

}}
