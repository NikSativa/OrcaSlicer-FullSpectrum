#include "Fs3mfIds.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

namespace Slic3r::FullSpectrum3mf {

std::string sanitize_id_fragment(const std::string &value)
{
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc))
            out.push_back(static_cast<char>(std::tolower(uc)));
        else if (c == '-' || c == '_')
            out.push_back(c);
        else if (!out.empty() && out.back() != '_')
            out.push_back('_');
    }

    while (!out.empty() && out.front() == '_')
        out.erase(out.begin());
    while (!out.empty() && out.back() == '_')
        out.pop_back();
    return out;
}

std::string short_stable_hash(const std::string &value)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(), digest);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < 8; ++i)
        ss << std::setw(2) << unsigned(digest[i]);
    return ss.str();
}

std::string make_stable_id(const std::string &prefix, const std::string &seed)
{
    const std::string sanitized = sanitize_id_fragment(seed);
    if (!sanitized.empty() && sanitized.size() <= 64)
        return prefix + "_" + sanitized;
    return prefix + "_" + short_stable_hash(seed);
}

std::string make_indexed_id(const std::string &prefix, size_t one_based_index)
{
    return prefix + "_" + std::to_string(one_based_index);
}

std::string physical_filament_id_from_source(size_t one_based_index,
                                             const std::string &filament_id,
                                             const std::string &preset_name,
                                             const std::string &color)
{
    if (!filament_id.empty())
        return make_stable_id("fil", filament_id);

    const std::string seed = std::to_string(one_based_index) + "|" + preset_name + "|" + color;
    return make_stable_id("fil", seed);
}

std::string mixed_filament_id_from_legacy_stable_id(uint64_t stable_id,
                                                    const std::string &fallback_seed)
{
    if (stable_id != 0)
        return "mix_" + std::to_string(stable_id);
    return make_stable_id("mix", fallback_seed);
}

} // namespace Slic3r::FullSpectrum3mf
