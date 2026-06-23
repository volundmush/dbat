return {
    id = "ethereal_armor",
    name = "Ethereal Armor",
    tags = { "ethereal_armor" },
    persistent = true,

    on_check_attack_defense = function(ch, cond, ctx)
        if ctx.absorbed or ctx.damage <= 0 then return end
        ctx.damage = math.floor(ctx.damage * 0.9)
    end,
}
