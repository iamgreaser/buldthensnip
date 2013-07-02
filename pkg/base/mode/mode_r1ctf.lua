-- R1CTF: It's not an intel, it's actually a bomb.
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

-- create our tent
dofile("pkg/base/mode/obj_tent.lua")
local s_new_tent = new_tent
local function f_new_tent(...)
	local this = s_new_tent(...)

	local s_player_in_range = this.player_in_range
	function this.player_in_range(plr, sec_current, ...)
		local ret = s_player_in_range(plr, sec_current, ...)

		if plr.has_intel and plr.team ~= this.team then
			plr.intel_capture(sec_current)
		end

		return ret
	end

	return this
end

-- apply 1ctf
dofile("pkg/base/mode/mode_1ctf.lua")

-- replace their tent with ours
new_tent = f_new_tent

-- recolour the intel
local s_new_intel = new_intel
function new_intel(...)
	local this = s_new_intel(...)

	function this.get_name()
		return "bomb"
	end

	this.color = {255,0,0}
	this.color_icon = {255,0,0}

	if client then
		l = this.color
		local mname, mdata
		mname,mdata = common.model_bone_get(mdl_intel, 0)
		print(l[1],l[2],l[3])
		recolor_component(l[1],l[2],l[3],mdata)
		common.model_bone_set(this.mdl_intel, 0, mname, mdata)
	end

	return this
end

local s_new_player = new_player
function new_player(...)
	local this = s_new_player(...)

	function this.intel_capture(sec_current)
		if server then
			local intel = this.has_intel
			if not intel then
				return
			end
			
			intel.intel_capture(sec_current)
			this.has_intel = nil
			
			local s = "* "..this.name.." has delivered the "..intel.get_name().."."
			net_broadcast(nil, common.net_pack("BIz", PKT_CHAT_ADD_TEXT, 0xFF800000, s))
			net_broadcast_team(this.team, common.net_pack("B", PKT_MAP_RCIRC))
		end
	end

	return this
end

