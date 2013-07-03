--[[
    This file is part of Ice Lua Components.

    Ice Lua Components is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ice Lua Components is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ice Lua Components.  If not, see <http://www.gnu.org/licenses/>.
]]

if client then
	weapon_models[WPN_RIFLE] = skin_load("pmf", "rifle.pmf", DIR_PKG_PMF)
end

return function (plr)
	local this = tpl_gun(plr, {
		dmg = {
			head = 100,
			body = 49,
			legs = 33,
		},
		
		ammo_clip = 10,
		ammo_reserve = 50,
		time_fire = 1/2,
		time_reload = 2.5,
		
		recoil_x = 0.0001,
		recoil_y = -0.05,

		model = weapon_models[WPN_RIFLE],
		
		name = "Rifle",
	})
	
	return this
end

