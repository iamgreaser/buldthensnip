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

commands = {}
command_color = {
	error = 0xFFFF6666,
	usage = 0xFF6666FF,
	success = 0xFF66FF66,
	text = 0xFFDDDDFF,
}

function command_deregister(command)
	if commands[command] ~= nil then
		commands[command] = nil
	end
end

function command_msg(status, neth, msg)
	if type(neth) == "string" then
		irc.write(neth..": "..msg)
	else
		net_send(neth, common.net_pack("BIz", PKT_CHAT_ADD_TEXT, command_color[status], msg))
	end
end

function command_register(settings)
	local this = {
		command = string.lower(settings.command),
		permission = settings.permission,
		usage = settings.usage,
		func = settings.func
	} this.this = this
	
	function this.exec(player, plrid, neth, params, msg)
		if this.permission == nil or (not player) or player.has_permission(this.permission) then
			this.func(player, plrid, neth, params, msg)
		else
			command_msg("error", neth, "Error: You do not have permission for this command")
		end
	end
	
	commands[this.command] = this
end

--You have to deregister aliases separately
function command_register_alias(command, alias)
	commands[alias] = commands[command]
end

function command_handle(player, plrid, neth, params, msg)
	local status, emsg = pcall(function()
		local cmd = string.lower(params[1])
		if commands[cmd] ~= nil then
			table.remove(params, 1)
			commands[cmd].exec(player, plrid, neth, params, msg)
		else
			command_msg("error", neth, "Error: No such command")
		end
	end)

	if not status then
		print("ERROR on running command:", emsg)
		pcall(command_msg, "error", neth, emsg)
	end
end

command_register({
	command = "help",
	permission = nil,
	usage = "/help [command name]",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 0 then
			--TODO: List available commands
		elseif table.getn(prms) == 1 then
			if commands[prms[1]] == nil then
				command_msg("error", neth, "Error: No such command")
			elseif (not plr) or plr.has_permission(commands[prms[1]].permission) then
				command_msg("usage", neth, "Usage: "..commands[prms[1]].usage)
			else
				command_msg("error", neth, "Error: You do not have permission for this command")
			end
		else
			this.func(plr, plrid, neth, "help")
		end
	end
})

-- for testing. not like it can be abused. well, i hope it can't be abused. --GM
command_register({
	command = "kickme",
	permission = nil,
	usage = "/kickme",
	func = function(plr, plrid, neth, prms, msg)
		server.net_kick(neth, "requested!")
	end
})

command_register({
	command = "kick",
	permission = "kick",
	usage = "/kick <player>",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 1 then
			local target
			prms[1] = tostring(prms[1])
			if prms[1]:sub(0, 1) == "#" then
				target = players[tonumber(prms[1]:sub(2))]
			end
			for i=1,players.max do
				if players[i] ~= nil and players[i].name == prms[1] then
					target = players[i]
					break
				end
			end
			if target then
				server.net_kick(target.neth, "requested!")
			else
				command_msg("error", neth, "Error: Player not found")
			end
		else
			commands["help"].func(plr, plrid, neth, {"piano"})
		end
	end
})

command_register({
	command = "resetscore",
	permission = "resetgame",
	usage = "/resetscore",
	func = function(plr, plrid, neth, prms, msg)
		local i
		for i=1,players.max do
			if players[i] then
				players[i].score = 0
				players[i].kills = 0
				players[i].deaths = 0
				players[i].update_score()
			end
		end
	end
})

command_register({
	command = "me",
	permission = "me",
	usage = "/me <action>",
	func = function(plr, plrid, neth, prms, msg)
		if not plr then
			-- IRC - ignore
		elseif table.getn(prms) > 0 then
			local m = "* "..plr.name.." "..string.sub(msg,5)
			net_broadcast(nil, common.net_pack("BIz", PKT_CHAT_ADD_TEXT, 0xFFFFFFFF, m))
			irc.write(m)
		else
			commands["help"].func(plr, plrid, neth, {"me"})
		end 
	end
})

command_register({
	command = "fog",
	permission = "resetgame",
	usage = "/fog r g b",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 3 then
			fog_set(tonumber(prms[1]),
				tonumber(prms[2]),
				tonumber(prms[3]))
		else
			commands["help"].func(plr, plrid, neth, {"fog"})
		end
	end
})

command_register({
	command = "resetgame",
	permission = "resetgame",
	usage = "/resetgame",
	func = function(plr, plrid, neth, prms, msg)
		mode_reset()
	end
})

command_register({
	command = "map",
	permission = "map",
	usage = "/map <filename>",
	func = function(plr, plrid, neth, prms, msg)
		local status, err = pcall(function()
			local _map_loaded = common.map_load(prms[1], "auto")
			common.map_set(_map_loaded)
			map_loaded = _map_loaded
		end)
		if not status then
			command_msg("error", neth, "Error while loading map: " .. err)
			return
		end
		map_fname = prms[1]
		map_name = map_fname
		while map_name do
			local p = map_name:find("/", 1, true)
			if not p then break end
			map_name = map_name:sub(p+1)
		end
		net_broadcast(nil, common.net_pack("B", PKT_NEW_MAP))
		mode_reset()
	end
})

command_register({
	command = "genmap",
	permission = "map",
	usage = "/genmap <filename> <flags>",
	func = function(plr, plrid, neth, prms, msg)
		local status, err = pcall(function()
			local _map_loaded, map_name = loadfile(prms[1])(parse_commandline_options(prms))
			common.map_set(_map_loaded)
			map_loaded = _map_loaded
		end)
		if not status then
			command_msg("error", neth, "Error while loading map: " .. err)
			return
		end
		map_fname = prms[1]
		net_broadcast(nil, common.net_pack("B", PKT_NEW_MAP))
		mode_reset()
	end
})

command_register({
	command = "squad",
	permission = "squad",
	usage = "/squad <squad name> (Use \"none\" to leave your squad)",
	func = function(plr, plrid, neth, prms, msg)
		if not plr then
			-- IRC - ignore
		elseif table.getn(prms) > 0 then
			if prms[1] == "none" then
				plr.squad = nil
			else
				plr.squad = string.sub(msg,8)
			end
			plr.update_score()
		else
			commands["help"].func(plr, plrid, neth, {"squad"})
		end
	end
})

command_register({
	command = "kill",
	permission = "kill",
	usage = "/kill",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 0 then
			if plr then
				plr.set_health_damage(0, 0xFF800000, plr.name.." shuffled off this mortal coil", plr)
			end
		else
			commands["help"].func(plr, plrid, neth, {"kill"})
		end
	end
})

command_register({
	command = "gmode",
	permission = "gmode",
	usage = "/gmode #; where 1=normal, 2=spectate, 3=editor", 
	func = function(plr, plrid, neth, prms, msg)
		if not plr then
			-- IRC - ignore
		elseif table.getn(prms) == 1 then
			local n = math.floor(tonumber(prms[1]) or 0)
			if n >= 1 and n <= 3 then
				plr.mode = n
				plr.update_score()
			end
		else
			commands.help.func(plr, plrid, neth, {"gmode"})
		end
	end
})

command_register({
	command = "piano",
	permission = "piano",
	usage = "/piano <player>",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 1 then
			local target
			prms[1] = tostring(prms[1])
			if prms[1]:sub(0, 1) == "#" then
				target = players[tonumber(prms[1]:sub(2))]
			end
			for i=1,players.max do
				if players[i] ~= nil and players[i].name == prms[1] then
					target = players[i]
					break
				end
			end
			if target then
				target.drop_piano()
			else
				command_msg("error", neth, "Error: Player not found")
			end
		else
			commands["help"].func(plr, plrid, neth, {"piano"})
		end
	end
})
command_register({
	command = "teleport",
	permission = "teleport",
	usage = "/teleport <player>|<x y z>",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 1 then
			local target
			prms[1] = tostring(prms[1])
			if prms[1]:sub(0, 1) == "#" then
				target = players[tonumber(prms[1]:sub(2))]
			end
			for i=1,players.max do
				if players[i] ~= nil and players[i].name == prms[1] then
					target = players[i]
					break
				end
			end
			if target then
				local x, y, z = target.x, target.y, target.z
				plr.set_pos_recv(x, y, z)
				net_broadcast(nil, common.net_pack("BBffffff",
					PKT_PLR_POS, plrid, x, y, z, 0.0, 0.0, 0.0))
			else
				command_msg("error", neth, "Error: Player not found")
			end
		elseif table.getn(prms) == 3 then
			--NOTE: I protest that y is down/same way AoS was
			local x, y, z = tonumber(prms[1]), tonumber(prms[2]), tonumber(prms[3])
			plr.set_pos_recv(x, y, z)
			net_broadcast(nil, common.net_pack("BBffffff",
				PKT_PLR_POS, plrid, x, y, z, 0.0, 0.0, 0.0))
		else
			commands["help"].func(plr, plrid, neth, {"teleport"})
		end
	end
})
command_register_alias("teleport", "tp")

command_register({
	command = "goto",
	permission = "goto",
	usage = "/goto #X ; where # is letter, X is number in the map's grid system",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 1 then
			prms[1] = tostring(prms[1])
			local x, z
			local success = pcall(function()
				x = (string.byte(prms[1]:lower()) - 97) * 64 + 32
				z = (tonumber(prms[1]:sub(2, 2)) - 1) * 64 + 32
			end)
			local xlen, _, zlen = common.map_get_dims()
			if (success and x >= 0 and x < xlen and z >= 0 and z < zlen) then
				local y = common.map_pillar_get(x, z)[1+1] - 3
				plr.set_pos_recv(x, y, z)
				net_broadcast(nil, common.net_pack("BBffffff",
					PKT_PLR_POS, plrid, x + 0.5, y, z + 0.5, 0.0, 0.0 ,0.0))
			else
				command_msg("error", neth, "Error: Invalid coordinates")
			end
		else
			commands["help"].func(plr, plrid, neth, {"goto"})
		end
	end
})

command_register({
	command = "intel",
	permission = "intelcmd",
	usage = "/intel",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 0 then
			local i
			for i=1,#miscents do
				if miscents[i] ~= nil then
					local tidx = miscents[i].team
					local tname = (tidx and teams[tidx].name) or "Neutral"
					command_msg("text", neth, tname.." "..miscents[i].type..": "..miscents[i].x..", "..miscents[i].y..", "..miscents[i].z)
				end
			end
		else
			commands["help"].func(plr, plrid, neth, {"intel"})
		end
	end
})

command_register({
	command = "who",
	--permission = "irc",
	usage = "/who",
	func = function(plr, plrid, neth, prms, msg)
		local s = ""
		local pc = 0
		for i=1,players.max do
			if players[i] ~= nil then
				pc = pc + 1

				if s ~= "" then
					s = s..", "
				end

				s = s .. (
					"[" .. tostring(players[i].name) ..
					" #".. tostring(i) ..
					" T".. tostring(players[i].team) ..
					"]")

			end
		end
		s = tostring(pc) .. " players: " .. s
		command_msg("text", neth, s)
	end
})


command_register({
	command = "login",
	permission = nil,
	usage = "/login <group> <password>",
	func = function(plr, plrid, neth, prms, msg)
		if not plr then
			-- IRC - ignore
		elseif table.getn(prms) == 2 then
			local success = false
			if permissions[prms[1]] ~= nil and prms[2] == permissions[prms[1]].password then
				plr.add_permission_group(permissions[prms[1]].perms)
				success = true
			end
			if success then
				command_msg("success", neth, "You have successfully logged in as "..prms[1])
			else
				command_msg("error", neth, "Could not log in to group "..prms[1].." with that password")
			end
		else
			commands["help"].func(plr, plrid, neth, {"login"})
		end
	end
})

command_register({
	command = "permissions",
	permission = "permissions",
	usage = "/permissions <player> [add/remove] <permission>",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 3 then
			local target
			prms[1] = tostring(prms[1])
			if prms[1]:sub(0, 1) == "#" then
				target = players[tonumber(prms[1]:sub(2))]
			end
			for i=1,players.max do
				if players[i] ~= nil and players[i].name == prms[1] then
					target = players[i]
					break
				end
			end
			if not target then
				command_msg("error", neth, "Error: Player not found")
				return
			end
			local add_perm
			prms[2] = tostring(prms[2])
			if prms[2] == "add" or prms[2] == "a" or prms[2] == "+" then
				add_perm = true
			elseif prms[2] == "remove" or prms[2] == "rem" or prms[2] == "r" or prms[2] == "-" then
				add_perm = false
			else
				commands["help"].func(plr, plrid, neth, {"permissions"})
				return
			end
			prms[3] = tostring(prms[3])
			if add_perm then
				target.add_permission(prms[3])
			else
				target.remove_permission(prms[3])
			end
		else
			commands["help"].func(plr, plrid, neth, {"permissions"})
		end
	end
})
command_register_alias("permissions", "permission")
command_register_alias("permissions", "perms")
command_register_alias("permissions", "perm")

command_register({
	command = "logout",
	permission = "logout",
	usage = "/logout",
	func = function(plr, plrid, neth, prms, msg)
		if table.getn(prms) == 0 then
			plr.clear_permissions()
			plr.add_permission_group(permissions["default"].perms)
			command_msg("success", neth, "You have successfully logged out")
		else
			commands["help"].func(plr, plrid, neth, {"logout"})
		end
	end
})

