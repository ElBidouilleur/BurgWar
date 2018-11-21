WEAPON.Scale = 0.714
WEAPON.Sprite = "../resources/emmentalibur.png"
WEAPON.SpriteOrigin = Vec2(40, 284) * WEAPON.Scale
WEAPON.WeaponOffset = Vec2(30, -80) -- This should not be here
WEAPON.Cooldown = 0.3
WEAPON.Animations = {
	{"attack", 0.3}
}

function WEAPON:OnAttack()
	if (self:IsPlayingAnimation()) then
		return
	end

	local pos = self:GetPosition()
	local maxs = Vec2(160, 74)
	local mins = Vec2(40, -76)

	if (not self:IsLookingRight()) then
		maxs = maxs * -1
		mins = mins * -1
	end

	self:PlayAnim("attack")
	self:DealDamage(100, Rect(pos + mins, pos + maxs))
end

function WEAPON:PlayAnimation(animation)
	
end
