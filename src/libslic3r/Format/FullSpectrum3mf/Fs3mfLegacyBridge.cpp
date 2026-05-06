#include "Fs3mfLegacyBridge.hpp"

#include "Fs3mfConstants.hpp"
#include "Fs3mfIds.hpp"

#include "../../MixedFilament.hpp"
#include "../../PrintConfig.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace Slic3r::FullSpectrum3mf {

namespace {

std::vector<std::string> string_values(const DynamicPrintConfig &config, const std::string &key)
{
    if (const auto *opt = config.option<ConfigOptionStrings>(key); opt != nullptr)
        return opt->values;
    return {};
}

std::vector<double> double_values(const DynamicPrintConfig &config, const std::string &key)
{
    if (const auto *opt = config.option<ConfigOptionFloats>(key); opt != nullptr)
        return opt->values;
    return {};
}

std::string value_at(const std::vector<std::string> &values, size_t idx, const std::string &fallback = {})
{
    return idx < values.size() ? values[idx] : fallback;
}

std::string unique_id(const std::string &base_id, size_t one_based_index, std::unordered_set<std::string> &used_ids)
{
    if (used_ids.insert(base_id).second)
        return base_id;

    size_t attempt = 0;
    for (;;) {
        std::string candidate = base_id + "_" + std::to_string(one_based_index);
        if (attempt != 0)
            candidate += "_" + std::to_string(attempt);
        if (used_ids.insert(candidate).second)
            return candidate;
        ++attempt;
    }
}

double value_at(const std::vector<double> &values, size_t idx, double fallback)
{
    return idx < values.size() ? values[idx] : fallback;
}

std::vector<std::string> split_pattern_groups(const std::string &pattern)
{
    std::vector<std::string> groups;
    std::stringstream ss(pattern);
    std::string group;
    while (std::getline(ss, group, ',')) {
        if (!group.empty())
            groups.emplace_back(group);
    }
    return groups;
}

std::optional<ManualPattern> manual_pattern_from_legacy(const std::string              &pattern,
                                                        const std::vector<std::string> &physical_refs)
{
    const std::string normalized = MixedFilamentManager::normalize_manual_pattern(pattern);
    if (normalized.empty())
        return std::nullopt;

    ManualPattern out;
    for (const std::string &group : split_pattern_groups(normalized)) {
        std::vector<std::string> steps;
        steps.reserve(group.size());
        for (char c : group) {
            if (c == '1')
                steps.emplace_back("component_a");
            else if (c == '2')
                steps.emplace_back("component_b");
            else if (c >= '3' && c <= '9') {
                const size_t physical_idx = size_t(c - '1');
                if (physical_idx < physical_refs.size())
                    steps.emplace_back("physical:" + physical_refs[physical_idx]);
            }
        }
        if (!steps.empty())
            out.groups.emplace_back(std::move(steps));
    }

    return out.groups.empty() ? std::nullopt : std::optional<ManualPattern>(std::move(out));
}

std::string legacy_pattern_from_manual_pattern(const std::optional<ManualPattern> &manual_pattern,
                                               const std::vector<std::string>     &physical_refs)
{
    if (!manual_pattern || manual_pattern->groups.empty())
        return {};

    std::unordered_map<std::string, char> physical_token_by_ref;
    for (size_t i = 0; i < physical_refs.size() && i < 9; ++i)
        physical_token_by_ref.emplace(physical_refs[i], char('1' + i));

    std::ostringstream ss;
    for (size_t group_idx = 0; group_idx < manual_pattern->groups.size(); ++group_idx) {
        if (group_idx != 0)
            ss << ',';
        for (const std::string &step : manual_pattern->groups[group_idx]) {
            if (step == "component_a")
                ss << '1';
            else if (step == "component_b")
                ss << '2';
            else if (step.rfind("physical:", 0) == 0) {
                const std::string physical_ref = step.substr(9);
                auto it = physical_token_by_ref.find(physical_ref);
                if (it != physical_token_by_ref.end())
                    ss << it->second;
            }
        }
    }

    return MixedFilamentManager::normalize_manual_pattern(ss.str());
}

std::optional<Gradient> gradient_from_legacy(const std::string              &ids,
                                             const std::string              &weights,
                                             const std::vector<std::string> &physical_refs)
{
    Gradient gradient;
    for (char c : ids) {
        if (c < '1' || c > '9')
            continue;
        const size_t physical_idx = size_t(c - '1');
        if (physical_idx < physical_refs.size())
            gradient.component_refs.emplace_back(physical_refs[physical_idx]);
    }

    if (gradient.component_refs.size() < 3)
        return std::nullopt;

    std::stringstream ss(weights);
    std::string token;
    while (std::getline(ss, token, '/')) {
        try {
            gradient.weights.emplace_back(std::max(0, std::stoi(token)));
        } catch (...) {
            gradient.weights.emplace_back(0);
        }
    }
    if (gradient.weights.size() != gradient.component_refs.size())
        gradient.weights.assign(gradient.component_refs.size(), 1);

    return gradient;
}

std::string legacy_gradient_ids(const std::optional<Gradient>     &gradient,
                                const std::vector<std::string>    &physical_refs)
{
    if (!gradient || gradient->component_refs.size() < 3)
        return {};

    std::string out;
    for (const std::string &ref : gradient->component_refs) {
        auto it = std::find(physical_refs.begin(), physical_refs.end(), ref);
        if (it == physical_refs.end())
            continue;
        const size_t idx = size_t(std::distance(physical_refs.begin(), it));
        if (idx < 9)
            out.push_back(char('1' + idx));
    }
    return out.size() >= 3 ? out : std::string();
}

std::string legacy_gradient_weights(const std::optional<Gradient> &gradient)
{
    if (!gradient || gradient->weights.empty())
        return {};

    std::ostringstream ss;
    for (size_t i = 0; i < gradient->weights.size(); ++i) {
        if (i != 0)
            ss << '/';
        ss << std::max(0, gradient->weights[i]);
    }
    return ss.str();
}

std::string distribution_mode_from_legacy(int mode)
{
    if (mode == int(MixedFilament::LayerCycle))
        return "layer_cycle";
    return "simple";
}

int legacy_distribution_mode(const std::string &mode, bool has_gradient)
{
    if (mode == "layer_cycle" || mode == "height_weighted")
        return int(MixedFilament::LayerCycle);
    return has_gradient ? int(MixedFilament::LayerCycle) : int(MixedFilament::Simple);
}

unsigned int physical_index_from_ref(const std::string &ref, const std::vector<std::string> &physical_refs)
{
    auto it = std::find(physical_refs.begin(), physical_refs.end(), ref);
    if (it == physical_refs.end())
        return 0;
    return unsigned(std::distance(physical_refs.begin(), it) + 1);
}

} // namespace

Materials materials_from_project_config(const DynamicPrintConfig &config)
{
    Materials materials;
    materials.kind = KIND_MATERIALS;
    materials.schema_version = PROFILE_VERSION;

    std::vector<std::string> colours = string_values(config, "filament_colour");
    if (colours.empty())
        colours = string_values(config, "default_filament_colour");
    const std::vector<std::string> preset_names = string_values(config, "filament_settings_id");
    const std::vector<std::string> filament_ids = string_values(config, "filament_ids");
    const std::vector<std::string> filament_types = string_values(config, "filament_type");
    const std::vector<double> diameters = double_values(config, "filament_diameter");

    size_t count = std::max({colours.size(), preset_names.size(), filament_ids.size(), diameters.size()});
    if (count == 0)
        count = 1;

    materials.physical_filaments.reserve(count);
    std::unordered_set<std::string> used_ids;
    for (size_t i = 0; i < count; ++i) {
        PhysicalFilament filament;
        filament.source_index = i + 1;
        filament.color = value_at(colours, i, "#FFFFFF");
        filament.display_name = value_at(preset_names, i, "Filament " + std::to_string(i + 1));
        filament.material_family = value_at(filament_types, i, {});
        filament.diameter_mm = value_at(diameters, i, 1.75);
        filament.id = unique_id(physical_filament_id_from_source(i + 1,
                                                                 value_at(filament_ids, i, {}),
                                                                 filament.display_name,
                                                                 filament.color),
                                i + 1,
                                used_ids);
        materials.physical_filaments.emplace_back(std::move(filament));
    }

    return materials;
}

std::vector<std::string> physical_filament_refs(const Materials &materials)
{
    std::vector<std::string> refs;
    refs.reserve(materials.physical_filaments.size());
    for (const PhysicalFilament &filament : materials.physical_filaments)
        refs.emplace_back(filament.id);
    return refs;
}

MixedFilaments mixed_filaments_from_manager(const MixedFilamentManager    &manager,
                                            const std::vector<std::string> &physical_refs)
{
    MixedFilaments mixed;
    mixed.kind = KIND_MIXED_FILAMENTS;
    mixed.schema_version = PROFILE_VERSION;

    std::unordered_set<std::string> used_ids(physical_refs.begin(), physical_refs.end());
    size_t row_idx = 0;
    for (const MixedFilament &row : manager.mixed_filaments()) {
        ++row_idx;
        if (row.component_a == 0 || row.component_b == 0 ||
            row.component_a > physical_refs.size() || row.component_b > physical_refs.size())
            continue;

        VirtualFilament vf;
        vf.legacy_stable_id = row.stable_id;
        vf.id = unique_id(mixed_filament_id_from_legacy_stable_id(row.stable_id,
                                                                  std::to_string(row.component_a) + "|" +
                                                                  std::to_string(row.component_b) + "|" +
                                                                  std::to_string(row.mix_b_percent)),
                          row_idx,
                          used_ids);
        vf.enabled = row.enabled;
        vf.visibility_state = row.deleted ? "tombstoned" : "active";
        vf.source_kind = row.custom ? "custom" : "auto";
        vf.origin.component_refs = {physical_refs[row.component_a - 1], physical_refs[row.component_b - 1]};
        vf.origin.origin_auto_generated = row.origin_auto;
        vf.blend.component_b_percent = std::clamp(row.mix_b_percent, 0, 100);
        vf.distribution.mode = distribution_mode_from_legacy(row.distribution_mode);
        vf.manual_pattern = manual_pattern_from_legacy(row.manual_pattern, physical_refs);
        vf.gradient = gradient_from_legacy(row.gradient_component_ids, row.gradient_component_weights, physical_refs);
        vf.surface_bias.component_a_offset_mm = row.component_a_surface_offset;
        vf.surface_bias.component_b_offset_mm = row.component_b_surface_offset;
        if (row.local_z_max_sublayers > 0)
            vf.local_z = LocalZ{row.local_z_max_sublayers, "standard-pair-split"};
        mixed.virtual_filaments.emplace_back(std::move(vf));
    }

    return mixed;
}

MixedFilamentManager manager_from_mixed_filaments(const MixedFilaments          &mixed_filaments,
                                                  const std::vector<std::string> &filament_colours,
                                                  const std::vector<std::string> &physical_refs)
{
    MixedFilamentManager manager;
    std::vector<MixedFilament> &rows = manager.mixed_filaments();
    rows.clear();
    rows.reserve(mixed_filaments.virtual_filaments.size());

    for (const VirtualFilament &vf : mixed_filaments.virtual_filaments) {
        if (vf.origin.component_refs.size() < 2)
            continue;

        const unsigned int a = physical_index_from_ref(vf.origin.component_refs[0], physical_refs);
        const unsigned int b = physical_index_from_ref(vf.origin.component_refs[1], physical_refs);
        if (a == 0 || b == 0)
            continue;

        MixedFilament row;
        row.component_a = a;
        row.component_b = b;
        row.stable_id = vf.legacy_stable_id;
        if (row.stable_id == 0 && vf.id.rfind("mix_", 0) == 0) {
            try {
                row.stable_id = std::stoull(vf.id.substr(4));
            } catch (...) {
                row.stable_id = 0;
            }
        }
        row.enabled = vf.enabled;
        row.deleted = vf.visibility_state == "tombstoned";
        row.custom = vf.source_kind == "custom";
        row.origin_auto = vf.origin.origin_auto_generated;
        row.mix_b_percent = std::clamp(vf.blend.component_b_percent, 0, 100);
        row.manual_pattern = legacy_pattern_from_manual_pattern(vf.manual_pattern, physical_refs);
        row.gradient_component_ids = legacy_gradient_ids(vf.gradient, physical_refs);
        row.gradient_component_weights = legacy_gradient_weights(vf.gradient);
        row.distribution_mode = legacy_distribution_mode(vf.distribution.mode, vf.gradient.has_value());
        row.local_z_max_sublayers = vf.local_z ? std::max(0, vf.local_z->max_sublayers) : 0;
        row.component_a_surface_offset = float(vf.surface_bias.component_a_offset_mm);
        row.component_b_surface_offset = float(vf.surface_bias.component_b_offset_mm);
        rows.emplace_back(std::move(row));
    }

    manager.set_display_context(MixedFilamentDisplayContext{filament_colours.size(), filament_colours});
    return manager;
}

std::string legacy_rows_from_mixed_filaments(const MixedFilaments          &mixed_filaments,
                                             const std::vector<std::string> &physical_refs)
{
    std::vector<std::string> colours(physical_refs.size(), "#FFFFFF");
    MixedFilamentManager manager = manager_from_mixed_filaments(mixed_filaments, colours, physical_refs);
    return manager.serialize_custom_entries();
}

} // namespace Slic3r::FullSpectrum3mf
