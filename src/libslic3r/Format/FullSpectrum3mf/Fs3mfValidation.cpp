#include "Fs3mfValidation.hpp"

#include <set>

namespace Slic3r::FullSpectrum3mf {

void ValidationResult::fail(std::string message)
{
    valid = false;
    errors.emplace_back(std::move(message));
}

void ValidationResult::warn(std::string message)
{
    warnings.emplace_back(std::move(message));
}

ValidationResult validate_package_model(const PackageModel &model)
{
    ValidationResult result;

    std::set<std::string> material_refs;
    for (const PhysicalFilament &filament : model.materials.physical_filaments) {
        if (filament.id.empty())
            result.fail("physical filament has empty id");
        if (!material_refs.insert(filament.id).second)
            result.fail("duplicate material id: " + filament.id);
    }

    std::set<std::string> volume_refs;
    for (const VolumeBinding &binding : model.identity_map.volume_bindings) {
        if (binding.stable_volume_id.empty())
            result.fail("volume binding has empty stable volume id");
        volume_refs.insert(binding.stable_volume_id);
    }

    if (model.mixed_filaments) {
        for (const VirtualFilament &vf : model.mixed_filaments->virtual_filaments) {
            if (vf.id.empty())
                result.fail("virtual filament has empty id");
            if (!material_refs.insert(vf.id).second)
                result.fail("duplicate material id: " + vf.id);
            if (vf.origin.component_refs.size() < 2)
                result.fail("mixed filament " + vf.id + " has fewer than two components");
            for (const std::string &component_ref : vf.origin.component_refs) {
                bool found = false;
                for (const PhysicalFilament &filament : model.materials.physical_filaments) {
                    if (filament.id == component_ref) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    result.fail("mixed filament " + vf.id + " references missing physical filament " + component_ref);
            }
        }
    }

    for (const Assignment &assignment : model.assignments.assignments) {
        if (assignment.id.empty())
            result.fail("assignment has empty id");
        if (material_refs.count(assignment.material_ref) == 0)
            result.fail("assignment " + assignment.id + " references missing material " + assignment.material_ref);
        if (assignment.target.kind == "volume" && volume_refs.count(assignment.target.stable_volume_id) == 0)
            result.fail("assignment " + assignment.id + " references missing volume " + assignment.target.stable_volume_id);
    }

    for (const PaintStateBinding &binding : model.assignments.paint_state_bindings) {
        if (material_refs.count(binding.material_ref) == 0)
            result.fail("paint state binding references missing material " + binding.material_ref);
        if (volume_refs.count(binding.stable_volume_id) == 0)
            result.fail("paint state binding references missing volume " + binding.stable_volume_id);
    }

    return result;
}

} // namespace Slic3r::FullSpectrum3mf
