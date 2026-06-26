local M = {}

function M.base(ch, stat)
    if ch:condition_has("multiform") then
        local original_id = ch:condition_number_get("multiform", "original_id")
        local original = require("dbat").characters.by_id(original_id)
        if original ~= nil and original:id_get() ~= ch:id_get() then
            return original:der_base(stat)
        end
    end

    local value = ch:stat_get(stat)
    if ch:condition_has("multiform_original") then
        local clones = ch:clone_count()
        if clones < 0 then clones = 0 end
        return value // (clones + 1)
    end
    return value
end

return M
