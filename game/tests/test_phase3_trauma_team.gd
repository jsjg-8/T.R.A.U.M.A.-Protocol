extends SceneTree

# Phase 3 test: Trauma Team POC
# Full mission loop: INSERT → LOCATE VIP → STABILIZE → ESCORT → EXTRACT
# Then test with Director disruption
# Run locally (NOT in CI): godot --headless --path game/ -s tests/test_phase3_trauma_team.gd
#
# NOTE: sim enums (MissionStatus, ObjectiveType, PatientState,
# DirectorEventType) are pure C++ (enum class, not Variant-compatible),
# so GDScript uses int constants mirroring src/*.h order + the *_id /
# *_with_ids accessors. Values must match declaration order in:
#   src/mission.h (MissionStatus, ObjectiveType)
#   src/patient.h (PatientState)
#   src/director.h (DirectorEventType)

# MissionStatus
const MS_PENDING = 0
const MS_IN_PROGRESS = 1
const MS_COMPLETED_SUCCESS = 2
const MS_COMPLETED_FAILURE = 3
const MS_CANCELLED = 4
# ObjectiveType
const OBJ_LOCATE_VIP = 0
const OBJ_STABILIZE_PATIENT = 1
const OBJ_ESCORT_TO_EXTRACTION = 2
const OBJ_EXTRACT = 3
# PatientState
const PS_CRITICAL = 0
const PS_STABILIZED = 1
const PS_TRANSPORTABLE = 2
const PS_EXTRACTED = 3
const PS_DEAD = 4
# DirectorEventType
const EVT_POWER_FAILURE = 0
const EVT_REINFORCEMENT = 1
const EVT_AMBUSH = 2
const EVT_LOCKDOWN = 3

var pass_count = 0
var fail_count = 0

func _init():
	print("=== Phase 3: Trauma Team POC Test ===")

	# --- Test 1: Full mission without Director ---
	print("--- Test 1: Mission without Director ---")
	var ws = WorldSimulation.new()
	var mission = Mission.new()
	var patient = Patient.new()
	var campaign = CampaignState.new()

	# Setup mission: INSERT → LOCATE → STABILIZE → ESCORT → EXTRACT
	mission.set_insertion_room(1)
	mission.set_extraction_room(5)
	mission.add_objective_with_ids(OBJ_LOCATE_VIP, 3, 0)
	mission.add_objective_with_ids(OBJ_STABILIZE_PATIENT, 0, 0)
	mission.add_objective_with_ids(OBJ_ESCORT_TO_EXTRACTION, 5, 0)
	mission.add_objective_with_ids(OBJ_EXTRACT, 5, 0)

	# Setup patient
	patient.set_patient_agent_id(100)
	patient.set_state_id(PS_CRITICAL)

	# Create VIP agent
	var vip = Agent.new()
	vip.set_agent_id(100)
	vip.set_position(Vector3(15, 0, 0))
	vip.set_health(30.0)
	vip.set_max_health(100.0)
	ws.add_agent(vip)

	# Create trauma team (3 agents)
	for i in range(3):
		var a = Agent.new()
		a.set_agent_id(1 + i)
		a.set_faction_id(2)
		a.set_position(Vector3(0, 0, 0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		ws.add_agent(a)

	mission.start_mission()
	assert_eq(mission.get_status_id(), MS_IN_PROGRESS, "Mission should be IN_PROGRESS")

	# Simulate mission progression — tick sim + pump mission clock manually
	# (nodes are not in the scene tree, so _process never runs on its own).
	for tick in range(600):
		ws.tick(1.0 / 60.0)
		mission._process(1.0 / 60.0)

		if mission.get_status_id() == MS_IN_PROGRESS:
			mission.complete_current_objective()

	# Verify mission completed
	assert_eq(mission.get_status_id(), MS_COMPLETED_SUCCESS, "Mission should succeed")
	assert_gt(mission.get_elapsed_time(), 0.0, "Mission elapsed time should be > 0")

	# Record outcome in campaign
	campaign.record_mission_outcome(mission.get_mission_id(), true, 0, 2)
	assert_eq(campaign.get_total_missions(), 1, "Should have 1 mission recorded")
	assert_eq(campaign.get_successful_missions(), 1, "Should have 1 successful mission")

	# --- Test 2: Patient stabilization flow ---
	print("--- Test 2: Patient Stabilization ---")
	var patient2 = Patient.new()
	patient2.set_patient_agent_id(200)
	patient2.set_state_id(PS_CRITICAL)
	patient2.set_stabilization_required(2.0)

	patient2.start_stabilization(1)
	assert_eq(patient2.get_assigned_medic(), 1, "Medic should be assigned")

	# Tick stabilization
	for i in range(120):
		patient2.update_stabilization(1.0 / 60.0)

	assert_eq(patient2.get_state_id(), PS_STABILIZED, "Patient should be STABILIZED after 2s")
	assert_true(patient2.get_stabilization_progress() >= 1.0, "Stabilization progress should be >= 1.0")

	# Extract
	patient2.extract()
	assert_eq(patient2.get_state_id(), PS_EXTRACTED, "Patient should be EXTRACTED")

	# --- Test 3: Mission failure on time limit ---
	print("--- Test 3: Mission Time Limit ---")
	var mission3 = Mission.new()
	mission3.set_time_limit(1.0)
	mission3.add_objective_with_ids(OBJ_LOCATE_VIP, 3, 0)
	mission3.start_mission()

	# Tick past time limit — mission._process handles fail
	for i in range(120):
		mission3._process(1.0 / 60.0)

	assert_eq(mission3.get_status_id(), MS_COMPLETED_FAILURE, "Mission should fail on time limit")

	# --- Test 4: Mission without objectives stays IN_PROGRESS ---
	print("--- Test 4: Mission No Objectives ---")
	var mission4 = Mission.new()
	mission4.start_mission()
	assert_eq(mission4.get_status_id(), MS_IN_PROGRESS, "Mission without objectives should be IN_PROGRESS")

	# complete_current_objective with no objectives does nothing
	mission4.complete_current_objective()
	assert_eq(mission4.get_status_id(), MS_IN_PROGRESS, "Still IN_PROGRESS with no objectives")

	# --- Test 5: Director event emission via tick wiring ---
	print("--- Test 5: Director Events ---")
	var ws5 = WorldSimulation.new()
	var director = Director.new()
	# Director observes through the tick (next-tick deferred events) —
	# no manual observe() calls, no get_current_state (pure C++).
	ws5.set_director(director)

	# Create agents to populate world state
	for i in range(4):
		var a = Agent.new()
		a.set_agent_id(50 + i)
		a.set_faction_id(1)
		a.set_position(Vector3(10, 0, 0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		ws5.add_agent(a)

	# Tick and let Director observe through WorldSimulation
	for tick in range(300):
		ws5.tick(1.0 / 60.0)

		# Manually emit lockdown at tick 180
		if tick == 180:
			director.emit_event_with_id(EVT_LOCKDOWN, Vector3(10, 0, 0), 20.0, "Lockdown initiated")

	# Verify Director emitted events
	var has_lockdown = false
	var has_reinforcement = false
	for i in range(director.get_history_count()):
		var t = director.get_history_event_type(i)
		if t == EVT_LOCKDOWN:
			has_lockdown = true
		if t == EVT_REINFORCEMENT:
			has_reinforcement = true

	assert_true(has_lockdown, "Director should have emitted LOCKDOWN event")
	# REINFORCEMENT fires at mission_time_threshold=30s; 300 ticks × 1/60 = 5s — not enough
	assert_false(has_reinforcement, "Director should NOT have emitted REINFORCEMENT yet (only 5s elapsed)")

	# Tick longer to trigger time-based event
	for tick in range(2000):
		ws5.tick(1.0 / 60.0)

	for i in range(director.get_history_count()):
		if director.get_history_event_type(i) == EVT_REINFORCEMENT:
			has_reinforcement = true
			break
	assert_true(has_reinforcement, "Director should have emitted REINFORCEMENT after 30s+")

	# --- Test 6: Campaign state tracking ---
	print("--- Test 6: Campaign State ---")
	var campaign6 = CampaignState.new()
	campaign6.record_mission_outcome(1, true, 0, 3)
	campaign6.record_mission_outcome(2, false, 2, 1)
	campaign6.record_mission_outcome(3, true, 1, 2)

	assert_eq(campaign6.get_total_missions(), 3, "Should have 3 missions")
	assert_eq(campaign6.get_successful_missions(), 2, "Should have 2 successful missions")

	# Agent tracking
	campaign6.record_agent_death(300)
	campaign6.record_agent_extraction(301)
	assert_false(campaign6.is_agent_alive(300), "Dead agent should not be alive")
	assert_true(campaign6.is_agent_alive(301), "Extracted agent should be alive")
	assert_true(campaign6.is_agent_alive(999), "Unknown agent assumed alive")

	# District control
	campaign6.set_district_control(1, 2, 0.8)
	assert_eq(campaign6.get_controlling_faction(1), 2, "District 1 controlled by faction 2")
	assert_true(
		abs(campaign6.get_district_stability(1) - 0.8) < 0.01,
		"District 1 stability should be 0.8"
	)

	# Reputation
	campaign6.set_faction_reputation(2, 0.5)
	assert_true(
		abs(campaign6.get_faction_reputation(2) - 0.5) < 0.01,
		"Faction 2 reputation should be 0.5"
	)

	# Summary
	print("=== Phase 3: PASSED ", pass_count, "/", pass_count + fail_count, " ===")
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
		print("FAIL: ", msg)

func assert_false(val, msg: String):
	if not val:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg)
