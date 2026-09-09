extends SceneTree

# Headless tick harness — runs full WorldSimulation tick cycle without scene tree
# Proves: Agent movement, health, double-buffer state sync, dead agent no-update
# Run: godot --headless --path game/ -s tests/test_headless_tick.gd

var pass_count = 0
var fail_count = 0

func _init():
	print("=== Headless Tick Harness ===")

	var ws = WorldSimulation.new()
	ws.set_debug_verbose(false)

	# Create agent with move target
	var agent = Agent.new()
	agent.set_agent_id(1)
	agent.set_faction_id(1)
	agent.set_position(Vector3(0, 0, 0))
	agent.set_health(100.0)
	agent.set_max_health(100.0)
	ws.add_agent(agent)

	assert_eq(ws.get_agent_count(), 1, "Agent count should be 1")

	# Give agent a move target and tick
	agent.set_move_target(Vector3(10, 0, 0))
	for i in range(1000):
		ws.tick(1.0 / 60.0)

	# Verify movement
	var pos = agent.get_position()
	assert_gt(pos.x, 0.0, "Agent should have moved toward target")
	assert_lt(pos.x, 10.1, "Agent should not overshoot target")

	# Verify world state
	var state = ws.get_current_state()
	assert_eq(state.agents.size(), 1, "WorldState should have 1 agent")
	assert_eq(state.tick_count, 1000, "Tick count should be 1000")
	assert_gt(state.elapsed_time, 0.0, "Elapsed time should be > 0")

	# Verify state sync — position in WorldState matches agent
	assert_true(
		state.agents[0].position.is_equal_approx(pos),
		"WorldState position should match agent position"
	)

	# Test health
	agent.take_damage(50.0)
	assert_eq(agent.get_health(), 50.0, "Health should be 50 after 50 damage")
	assert_true(agent.get_is_alive(), "Agent should still be alive")

	agent.take_damage(60.0)
	assert_eq(agent.get_health(), 0.0, "Health should be 0 after lethal damage")
	assert_false(agent.get_is_alive(), "Agent should be dead")

	# Dead agent should not update
	var dead_pos = agent.get_position()
	agent.set_move_target(Vector3(20, 0, 0))
	ws.tick(1.0 / 60.0)
	assert_eq(agent.get_position(), dead_pos, "Dead agent should not move")

	# Multi-agent tick
	var ws2 = WorldSimulation.new()
	for i in range(10):
		var a = Agent.new()
		a.set_agent_id(100 + i)
		a.set_faction_id(2)
		a.set_position(Vector3(i * 5.0, 0, 0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		a.set_move_target(Vector3(i * 5.0 + 2, 0, 0))
		ws2.add_agent(a)

	assert_eq(ws2.get_agent_count(), 10, "Should have 10 agents")

	for i in range(600):
		ws2.tick(1.0 / 60.0)

	# All agents should have moved slightly
	var all_moved = true
	for i in range(10):
		var a = ws2.get_agent(100 + i)
		if a and a.get_position().x <= i * 5.0:
			all_moved = false
			break
	assert_true(all_moved, "All agents should have moved toward targets")

	# Summary
	print("=== Headless Tick Harness: PASSED ", pass_count, "/", pass_count + fail_count, " ===")
	if fail_count > 0:
		print("FAILURES: ", fail_count)
		quit(1)
	else:
		quit(0)

func assert_eq(actual, expected, msg: String):
	if actual == expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected ", expected, ", got ", actual)

func assert_gt(actual, expected, msg: String):
	if actual > expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected > ", expected, ", got ", actual)

func assert_lt(actual, expected, msg: String):
	if actual < expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected < ", expected, ", got ", actual)

func assert_true(val, msg: String):
	if val:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected true, got false")

func assert_false(val, msg: String):
	if not val:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected false, got true")
