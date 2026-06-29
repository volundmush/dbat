return {
    id         = "bio_android_regen",
    name       = "Bio-Android Regeneration",
    tags       = { "goop", "regenerating", "bio_android_regen" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:schedule_event("stage_30", 4000, 0)
        cond:schedule_event("stage_15", 34000, 0)
    end,

    on_event = function(ch, cond, event)
        if event == "stage_30" then
            ch:act("@GFrom the collection of cells growing a crude form of your body starts to take shape!@n",
                true, nil, nil, "char")
            ch:act("@GYou start to notice a large mass of pulsing flesh growing before you!@n",
                true, nil, nil, "room")
        elseif event == "stage_15" then
            ch:act("@GYour body has almost reached its previous form! Only a little more regenerating is needed!@n",
                true, nil, nil, "char")
            ch:act("@GThe lump of flesh has now grown to the size where the likeness of @g$n@G can be seen of it! It appears that $e is regenerating $s body from what was only a few cells!@n",
                true, nil, nil, "room")
        end
    end,

    on_remove = function(ch, cond, reason)
        cond:cancel_event("stage_30")
        cond:cancel_event("stage_15")
        if reason ~= "expired" then return end

        ch:meter_set("powerlevel", 1000000)
        ch:act("@GYour body has fully regenerated! You flex your arms and legs outward with a rush of renewed strength!@n",
            true, nil, nil, "char")
        ch:act("@g$n@G's body has fully regenerated! Suddenly $e flexes $s arms and legs and a rush of power erupts from off of $s body!@n",
            true, nil, nil, "room")
    end,
}
