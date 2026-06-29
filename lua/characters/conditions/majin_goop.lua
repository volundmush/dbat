return {
    id         = "majin_goop",
    name       = "Majin Goop",
    tags       = { "goop", "regenerating", "majin_goop" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:schedule_event("stage_30", 4000, 0)
        cond:schedule_event("stage_15", 34000, 0)
    end,

    on_event = function(ch, cond, event)
        if event == "stage_30" then
            ch:act("@MYou will the various chunks of your body to return and slowly more and more of them begin to fly into you. Your body begins to grow larger and larger as this process unfolds!@n ",
                true, nil, nil, "char")
            ch:act("@MThe various chunks of @m$n@M's body start to fly into the largest chunk! As the chunks collide they begin to form a larger and still growing blob of goo!@n",
                true, nil, nil, "room")
        elseif event == "stage_15" then
            ch:act("@MYour body has reached half its previous size as your limbs ooze slowly out into their proper shape!@n",
                true, nil, nil, "char")
            ch:act("@m$n@M's body has regenerated to half its previous size! Slowly $s limbs ooze out into their proper shape! It won't be long now till $e has fully regenerated!@n",
                true, nil, nil, "room")
        end
    end,

    on_remove = function(ch, cond, reason)
        cond:cancel_event("stage_30")
        cond:cancel_event("stage_15")
        if reason ~= "expired" then return end

        ch:meter_set("powerlevel", 1000000)
        ch:act("@MYour body has fully regenerated! You scream out in triumph and a short gust of steam erupts from your pores!@n",
            true, nil, nil, "char")
        ch:act("@m$n@M's body has fully regenerated! Suddenly $e screams out in gleeful triumph and short gust of steam erupts from $s skin pores!",
            true, nil, nil, "room")
    end,
}
