# Integrating Box2D 3.1.1 into the Mupfel ECS

A design guide for `Core/Source/Physics/`. Written against the vendored
`Vendor/Sources/box2d-3.1.1` headers and sources, not just the website — several pages on
box2d.org still show the v3.0 API (`bodyDef.angle`, which is `bodyDef.rotation` in 3.1).

The two goals stated up front, because every trade-off below resolves against them:

- **Performance**: no per-frame allocation, no mirroring of state that Box2D already owns, no
  `b2Body_GetTransform()` in a loop.
- **User-facing simplicity**: adding physics to an entity should be two `AddComponent` calls and
  nothing else. No `b2` type should ever appear in `App` code.

---

## 1. Ground rules Box2D imposes

These come straight from the docs/headers and they dictate the architecture:

| Rule | Source | Consequence for Mupfel |
|---|---|---|
| Not thread-safe; read-only queries are safe from multiple threads | FAQ | Every `b2Create*`/`b2Destroy*`/`b2Body_Set*` call must be serialized. `Registry::ParallelForEach` must never touch a `b2` mutator. |
| Defs (`b2WorldDef`, `b2BodyDef`, `b2ShapeDef`) are temporary, fully copied into internals | `simulation.md` | Stack-allocate them. Never store a def in a component. |
| Ids (`b2BodyId`, `b2ShapeId`) are opaque handles, passed by value, zero == null | `include/box2d/id.h` | Safe to store in a plain `std::vector` and to defer across frames. `b2BodyId` is 8 bytes. |
| MKS units; moving objects should be 0.1–10 m, 1 m is the sweet spot | FAQ | See §2 — you get this for free. |
| Radians, not degrees | `simulation.md` | Matches `Transform::rotation` already. |
| `b2World_GetBodyEvents` delivers moved bodies as a contiguous array and is the *recommended* way to update game-object transforms | `types.h:1126-1135` | This is the readback path. See §7. |
| Bodies created at the origin then moved cost ~2× | `types.h:177-179` | Always fill `bodyDef.position` from `Transform` before `b2CreateBody`. |
| Up to 128 worlds, non-interacting, simulatable in parallel | `simulation.md` | `Scene::MAX_SCENES == 64`, so one world per scene fits exactly. See §11. |

---

## 2. Units and axes — you need no scale factor

The classic Box2D integration headache is the pixels↔meters conversion. **Mupfel does not have
this problem**, and it is worth writing the invariant down before someone introduces one.

`Transform::pos_x/pos_y` are already world units, and the existing content is already MKS-shaped:
the ground quad in `App/Source/Level.cpp:31-32` is 31×13 units, entities sit ~1 unit apart, and
the player walks at 2 units/sec (`Player.cpp:132`). Declaring **1 world unit = 1 metre** puts
everything inside Box2D's 0.1–10 m sweet spot with a conversion factor of exactly 1.0.

The axis mapping:

```
Mupfel                          Box2D
Transform::pos_x        <->     b2Transform::p.x
Transform::pos_y        <->     b2Transform::p.y
Transform::pos_z                (not simulated — render layering only)
Transform::rotation     <->     b2Rot_GetAngle(b2Transform::q)
Transform::scale_x/_y           (sprite quad size — NOT collider extents, see §4)
```

`Transform::pos_z` must stay outside the simulation. The camera looks down the Z axis at the XY
plane (`Level.cpp:9`, pitch ≈ π/2), so Z is depth sorting, not height.

### Gravity must default to zero

`b2DefaultWorldDef()` sets `gravity = {0, -10}` (`src/types.c:12-13`). For a top-down camera that
makes every dynamic body slide toward the bottom of the screen forever. `CollisionSystem::Init`
currently tries to set exactly that. **Default to `{0.0f, 0.0f}`** and expose gravity as a
per-scene setting for side-scrollers.

---

## 3. The ownership rule

> Box2D owns simulated state. The ECS owns the *description* of the body and nothing else.

Concretely, once a body exists:

- `Transform` becomes **write-only for physics** — the simulation writes into it, nobody else does.
- Position/rotation/velocity live in Box2D. Do **not** add `velocity_x` to a component and copy it
  in and out each frame; that's two cache-missing scatter/gathers per body per frame to duplicate
  data Box2D already keeps hot.
- `Collider`/`RigidBody` are read at body-creation time and then only re-read if the user mutates
  them (which the engine can treat as "rebuild this body", or simply document as unsupported after
  creation for v1).

The failure mode to avoid is a "sync both directions every frame" system. It is the single biggest
performance mistake in ECS/Box2D integrations and it also creates ambiguity about who wins when
both sides changed.

---

## 4. Component design

Three public component types, plus zero private ones (see §5 for why the handle is not a
component).

### `Core/Include/ECS/Components/RigidBody.h` (new)

```cpp
#pragma once
#include <cstdint>

namespace Mupfel
{

enum class BodyType : uint8_t
{
    Static,    /**< Never moves. Zero mass. Cheapest. The default. */
    Kinematic, /**< Moved by velocity you set; ignores forces and collisions. */
    Dynamic,   /**< Fully simulated: forces, gravity, collision response. */
};

/**
 * Marks an entity as participating in the physics simulation. Needs a Transform (initial
 * placement) and a Collider (shape) on the same entity before a body is created.
 */
struct RigidBody
{
    BodyType type = BodyType::Static;
    float    gravity_scale   = 1.0f;
    float    linear_damping  = 0.0f;
    float    angular_damping = 0.0f;
    bool     fixed_rotation  = false; /**< Keep sprites axis-aligned. Very common for characters. */
    bool     is_bullet       = false; /**< Continuous collision. Costs extra; only for fast movers. */
    bool     allow_sleep     = true;
};

} // namespace Mupfel
```

Field-for-field these map onto `b2BodyDef` (`types.h:172`), so the translation function is trivial
and there is nothing for a user to learn beyond the doc comments.

### `Core/Include/ECS/Components/Collider.h` (currently an empty class)

```cpp
#pragma once
#include <cstdint>

namespace Mupfel
{

enum class ColliderShape : uint8_t
{
    Box,
    Circle,
    Capsule,
};

struct Collider
{
    ColliderShape shape = ColliderShape::Box;

    /** Box: half-width/half-height. Circle: `half_width` is the radius. Capsule: radius + half-height. */
    float half_width  = 0.5f;
    float half_height = 0.5f;

    /** Offset of the shape from the body origin, in world units. */
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    float density     = 1.0f;
    float friction    = 0.6f;  /**< Matches b2DefaultShapeDef(). */
    float restitution = 0.0f;  /**< Bounciness, [0,1]. */

    bool is_sensor       = false; /**< Overlap reports, no collision response. */
    bool report_contacts = false; /**< Emit CollisionBeganEvent/CollisionEndedEvent for this shape. */

    uint64_t category = 1;                 /**< b2Filter::categoryBits */
    uint64_t mask     = ~uint64_t{0};      /**< b2Filter::maskBits */
};

} // namespace Mupfel
```

**Why collider extents are separate from `Transform::scale_x/scale_y`.** Tempting to derive them —
don't. Sprites carry transparent padding (the vampire sprite in `Player.cpp:42-43` uses
`scale = 5.0` for a character that should collide as roughly one unit), and top-down games almost
always want a collider smaller than the sprite, often just the feet. Keeping them independent also
means changing a sprite's size never silently changes gameplay. Document the relationship instead
of encoding it.

**Why `report_contacts` defaults to false.** `b2DefaultShapeDef()` leaves `enableContactEvents`
zero-initialised to `false` (`src/types.c:55-65`) — Box2D 3.1 makes you opt in, and generating
begin/end touch events for every shape in a level is real per-frame cost. Mirror that default.

### Gotchas worth putting in the doc comments

- **Contact events fire if *either* shape enables them** (`src/contact.c:253`:
  `shapeA->enableContactEvents || shapeB->enableContactEvents`).
- **Sensor events require *both* shapes to enable them** — the sensor
  (`src/sensor.c:158`) *and* the visitor (`src/sensor.c:66`). This asymmetry is the #1 cause of
  "my trigger volume doesn't fire". Consider having the engine force `enable_sensor_events = true`
  on every non-sensor shape whose body is dynamic, so users only set the flag on the sensor. That
  costs a little broadphase work but removes a whole class of support questions.

---

## 5. Where the `b2BodyId` lives — and why not in a component

You need a two-way map: entity → body (to apply an impulse) and body → entity (to write the
transform back).

### Entity → body: a dense side table inside `CollisionSystem`

```cpp
// CollisionSystem.h, private
std::vector<b2BodyId> bodies;   /**< Indexed by Entity::Index(). b2_nullBodyId == no body. */
```

Not a component, for three reasons:

1. `Entity::MAX_COMPONENTS_TYPES` is 256 and `ComponentIndex` hands out ids from a global counter
   (`ComponentIndex.h:23`); a handle that no user ever queries shouldn't consume a signature bit.
2. Every `AddComponent` fires a `ComponentAddedEvent` through `EventSystem::AddImmediateEvent`
   (`Registry.h:260`). Creating a handle component per body means an immediate event per body for
   nobody's benefit.
3. It keeps `b2` types out of `Core/Include` entirely, which is what `App/Build-App.lua` relies on.

An `Entity::Index()`-indexed `std::vector` costs 8 bytes per *entity index ever allocated*, which is
nothing, and the lookup is a single bounds-checked load — faster than the sparse-set double
indirection a `ComponentArray` would give you.

### Body → entity: encode the entity index in `userData`

`b2BodyDef::userData` is a `void*` that comes back on every `b2BodyMoveEvent`
(`types.h:1136-1142`), which makes the readback loop a straight array walk with no map lookup.

**Do not store a pointer to a component.** `ComponentArray<T>` is a `std::vector`-backed sparse set
whose `Remove` does a swap-with-last (`ComponentArray.h:115-138`) and whose `Insert` does
`push_back` (`ComponentArray.h:105`). Component addresses are invalidated by any insert that
reallocates and by any removal of a *different* entity's component. A cached `Transform*` in
`userData` is a dangling pointer waiting for the next `AddComponent`.

Store the 4-byte entity index instead:

```cpp
static void* ToUserData(Entity e)
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(e.Index()));
}
```

### The friendship problem you will hit

`Entity`'s constructor is private and its friends are `EntityManager`, `View`, and `Registry`
(`Entity.h:24-31`). `CollisionSystem` is a friend of `Registry` (`Registry.h:36`) but **not** of
`Entity`, so `Entity{index}` will not compile inside `CollisionSystem.cpp`.

The minimal fix — no new friendship on `Entity`, reuses the friendship that already exists:

```cpp
// Registry.h, private section
/** Rebuilds an Entity from a raw index. For systems that round-trip indices through
    external libraries (e.g. Box2D user data). */
static constexpr Entity EntityFromIndex(uint32_t index) { return Entity{index}; }
```

`Registry` is a friend of `Entity` so it can construct one; `CollisionSystem` is a friend of
`Registry` so it can call this. (`Registry.h:29` already carries a `TODO: i do not want that!`
about the `View`-related friendship — this at least doesn't make that worse.)

---

## 6. Lifecycle: creating and destroying bodies

Bodies must be created and destroyed **outside** `b2World_Step` and **on one thread**. Both
`ComponentAddedEvent` and `EntityDestroyedEvent` are fired via `AddImmediateEvent`, i.e.
synchronously from wherever user code happens to be — possibly a `ParallelForEach` worker. So the
listeners must not call Box2D directly. Queue, then drain at the top of `Update`.

### Creation

Register once in `CollisionSystem::Init`:

```cpp
evt_system.RegisterListener<ComponentAddedEvent>(
    [this](const ComponentAddedEvent& ev)
    {
        static const Entity::Signature required =
            Registry::ComponentSignature<Transform, RigidBody, Collider>();

        if ((ev.sig & required) != required)
            return;                      // not physics-complete yet
        if (HasBody(ev.e))
            return;                      // already built

        std::scoped_lock lock(pending_mutex);
        pending_create.push_back(ev.e);
    });
```

Testing the *signature carried by the event* rather than the component id is what makes ordering
irrelevant: the body is built when the last of the three components arrives, whatever order the
user added them in.

### Destruction — queue the `b2BodyId`, not the entity

This one has a trap. `Registry::DestroyEntity` recycles the index into `EntityManager`'s free list
immediately, so an entity created later in the same frame can be handed the same index. A deferred
queue holding entity indices would then destroy the wrong body.

Look the body up at *event* time (a read from `bodies`, safe from any thread as long as the vector
isn't being resized concurrently — do the resize only on the main thread during drain), clear the
slot, and queue the id:

```cpp
evt_system.RegisterListener<EntityDestroyedEvent>(
    [this](const EntityDestroyedEvent& ev)
    {
        b2BodyId id = TakeBody(ev.e);        // reads + clears bodies[ev.e.Index()]
        if (B2_IS_NON_NULL(id))
        {
            std::scoped_lock lock(pending_mutex);
            pending_destroy.push_back(id);
        }
    });
```

`b2DestroyBody` destroys the body's shapes with it, so no separate shape bookkeeping is needed.
Also listen for `ComponentRemovedEvent` on `RigidBody`/`Collider` for the "entity survives, physics
goes away" case.

### Building the body

```cpp
void CollisionSystem::CreateBody(Entity e)
{
    const Transform& t  = registry.GetComponent<Transform>(e);
    const RigidBody& rb = registry.GetComponent<RigidBody>(e);
    const Collider&  c  = registry.GetComponent<Collider>(e);

    b2BodyDef def       = b2DefaultBodyDef();
    def.type            = ToB2(rb.type);
    def.position        = {t.pos_x, t.pos_y};        // never create at origin then move
    def.rotation        = b2MakeRot(t.rotation);
    def.gravityScale    = rb.gravity_scale;
    def.linearDamping   = rb.linear_damping;
    def.angularDamping  = rb.angular_damping;
    def.fixedRotation   = rb.fixed_rotation;
    def.isBullet        = rb.is_bullet;
    def.enableSleep     = rb.allow_sleep;
    def.userData        = ToUserData(e);

    b2BodyId body = b2CreateBody(worldId, &def);

    b2ShapeDef sd            = b2DefaultShapeDef();
    sd.density               = c.density;
    sd.material.friction     = c.friction;
    sd.material.restitution  = c.restitution;
    sd.isSensor              = c.is_sensor;
    sd.enableContactEvents   = c.report_contacts;
    sd.enableSensorEvents    = true;                 // see §4 gotcha
    sd.filter.categoryBits   = c.category;
    sd.filter.maskBits       = c.mask;
    sd.userData              = ToUserData(e);        // so contact events resolve to entities

    switch (c.shape)
    {
    case ColliderShape::Box:
    {
        b2Polygon box = b2MakeOffsetBox(c.half_width, c.half_height,
                                        {c.offset_x, c.offset_y}, b2Rot_identity);
        b2CreatePolygonShape(body, &sd, &box);
        break;
    }
    case ColliderShape::Circle:
    {
        b2Circle circle = {{c.offset_x, c.offset_y}, c.half_width};
        b2CreateCircleShape(body, &sd, &circle);
        break;
    }
    case ColliderShape::Capsule: /* b2Capsule + b2CreateCapsuleShape */ break;
    }

    SetBody(e, body);  // grows `bodies` if needed
}
```

Note `sd.userData` as well as `def.userData` — contact events give you `b2ShapeId`s, and
`b2Shape_GetUserData` (`box2d.h:525`) is one call versus `b2Shape_GetBody` +
`b2Body_GetUserData`.

Bulk-creating a level's static geometry: create all the static bodies with
`shapeDef.invokeContactCreation = false` (`types.h:398-401`) — it skips the environment scan per
shape, which the header calls out as a significant static-body creation cost.

---

## 7. Stepping and reading back

### The step: fixed timestep accumulator

Box2D wants a fixed timestep; `Application::Run` gives you a variable one. Put the accumulator in
`PhysicsSimulation` (it already owns `time_multi` and single-step, which fit naturally here):

```cpp
void PhysicsSimulation::Update(double elapsedTime)
{
    if (single_step) return;

    static constexpr float FIXED_DT  = 1.0f / 60.0f;
    static constexpr int   SUB_STEPS = 4;         // the recommended value, per simulation.md
    static constexpr double MAX_ACCUM = 0.25;     // avoid the spiral of death on a hitch

    accumulator = std::min(accumulator + elapsedTime * time_multi, MAX_ACCUM);

    while (accumulator >= FIXED_DT)
    {
        ProfilingSample prof("Box2D Step");
        collision_system->Step(FIXED_DT, SUB_STEPS);
        accumulator -= FIXED_DT;
    }

    collision_system->SyncTransforms();   // once, after the loop — see below
    collision_system->DispatchEvents();
}
```

`CollisionSystem::Step` drains the pending create/destroy queues, then calls
`b2World_Step(worldId, dt, subSteps)`.

**Why `SyncTransforms` runs once, not per step.** `b2World_Step` regenerates the move-event array
each call, so an inner-loop drain looks necessary. It isn't: `b2FinalizeBodiesTask`
(`src/solver.c:607-610`) writes a move event for *every* body in the awake set on every step, not
just ones that changed. A body that moved during step 1 is still awake for step 2 and so appears in
the final array; a body that fell asleep gets an event with `fellAsleep = true`. The last array is
therefore a superset of what you need.

### The readback

This is the hot loop, and it is why the `userData` design in §5 matters:

```cpp
void CollisionSystem::SyncTransforms()
{
    b2BodyEvents events = b2World_GetBodyEvents(worldId);
    auto&        transforms = registry.GetComponentArray<Transform>();

    for (int i = 0; i < events.moveCount; ++i)
    {
        const b2BodyMoveEvent& ev = events.moveEvents[i];

        Entity e = Registry::EntityFromIndex(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ev.userData)));

        if (!transforms.Has(e))     // component removed without the body being torn down
            continue;

        Transform& t = transforms.Get(e);
        t.pos_x    = ev.transform.p.x;
        t.pos_y    = ev.transform.p.y;
        t.rotation = b2Rot_GetAngle(ev.transform.q);
    }
}
```

Properties worth noting:

- The move-event array is **contiguous** and contains **only bodies that are awake**. Sleeping
  bodies cost nothing here — which is the strongest argument for leaving `worldDef.enableSleep`
  on for a game with a lot of settled static-ish geometry.
- The only random access is `transforms.Get(e)`, one sparse-set indirection per moved body.
- Compare to the naive alternative — `registry.view<RigidBody, Transform>()` plus
  `b2Body_GetTransform()` per entity — which touches every physics entity whether or not it moved
  and does a full body lookup each time. The docs explicitly recommend against it
  (`types.h:1132-1134`).
- Leave it single-threaded. Each event targets a distinct entity so it is *safe* to parallelise,
  but the loop is a few hundred nanoseconds for a thousand bodies and `ParallelForEach`'s
  future-per-chunk dispatch would dominate.

### Do not interpolate (yet)

A fixed 60 Hz step rendered at a variable rate produces mild temporal aliasing. The fix is storing
previous/current transforms and lerping at render time, which doubles `Transform` traffic. Ship
without it; add it only if it visibly bothers you.

---

## 8. Contact and sensor events → `EventSystem`

Translate Box2D's arrays into Mupfel events so `App` code never sees a `b2ShapeId`. New public
header, e.g. `Core/Include/ECS/PhysicsEvents.h`:

```cpp
class CollisionBeganEvent : public Event
{
public:
    CollisionBeganEvent(Entity in_a, Entity in_b) : a(in_a), b(in_b) {}
    Entity a, b;
};
// ... CollisionEndedEvent, SensorEnteredEvent { Entity sensor, visitor; }, SensorExitedEvent
```

```cpp
void CollisionSystem::DispatchEvents()
{
    b2ContactEvents contacts = b2World_GetContactEvents(worldId);
    for (int i = 0; i < contacts.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent& ev = contacts.beginEvents[i];
        evt_system.AddEvent<CollisionBeganEvent>(
            {EntityOf(ev.shapeIdA), EntityOf(ev.shapeIdB)});
    }
    // end events: shapes MAY already be destroyed -- guard with b2Shape_IsValid (types.h:1070)
    // sensor events: b2World_GetSensorEvents, same shape.
}
```

Two things to get right:

- **Use `AddEvent`, not `AddImmediateEvent`.** The event system is double-buffered, so a listener
  that reacts by destroying an entity does so next frame, outside the physics step. Immediate
  dispatch here means user callbacks running while you're mid-drain of Box2D's event arrays, which
  is exactly the reentrancy you want to avoid.
- **End-touch events can reference destroyed shapes.** `b2ContactEndTouchEvent` and
  `b2SensorEndTouchEvent` both carry the warning in `types.h:1067-1078` / `1017-1029`. Call
  `b2Shape_IsValid` (`box2d.h:504`) before `b2Shape_GetUserData`.

---

## 9. What happens to `MovementSystem`

Right now `MovementSystem::Move` is an empty stub (`MovementSystem.cpp:34-37`) and `Movement`
carries `velocity_x/y/z`, acceleration and friction. Once Box2D is in, having two systems that both
integrate velocity into `Transform` is a bug generator — whoever runs second wins.

The clean split:

- **Entity has a `RigidBody`** → Box2D owns its motion. `Movement` must be ignored. Applying
  velocity means `b2Body_SetLinearVelocity` (`box2d.h:271`) or an impulse; damping replaces
  `Movement::friction`; `Movement::acceleration_decay` has no Box2D equivalent and should be
  dropped for physics entities.
- **Entity has only `Movement`** → `MovementSystem` integrates it as it does today. This stays
  useful for things that need to move but not collide (camera rigs, floating UI markers, VFX), and
  it's the path that can genuinely go wide with `ParallelForEach`.

Enforce it in `PhysicsSimulation::Update` by having `MovementSystem` iterate
`view<Movement, Transform>()` and skip entities with a `RigidBody` signature bit, and log a warning
once if an entity has both. Silent precedence rules are worse than a warning.

`Player::UpdateMovement` (`Player.cpp:128-145`) writes `Transform` directly, which is the pattern
that has to change once the player gets a body — it becomes a velocity set on a kinematic or
dynamic body. That's the single most instructive migration to do first.

---

## 10. Multithreading — do this last, or not at all

`b2WorldDef` accepts `workerCount` + `enqueueTask` + `finishTask` + `userTaskContext`
(`types.h:120-136`). The temptation is to wire `ThreadPool` in immediately. Resist it:

- Box2D's own guidance is blunt: *"Do not modify the default value unless you are also providing a
  task system"*, and it notes that efficiency cores and hyper-threading *"provide little benefit and
  may even harm performance"* (`types.h:120-126`). Threading pays off in the thousands of bodies,
  not the dozens.
- The default (`workerCount = 0`, no callbacks) runs the solver serially and is correct.

**There is also a concrete blocker in the current `ThreadPool`.** `b2TaskCallback` is
`void(int startIndex, int endIndex, uint32_t workerIndex, void* taskContext)` (`types.h:33`) and
the header requires `workerIndex ∈ [0, workerCount)` with *"a worker must only exist on only one
thread at a time"*. Mupfel's `ThreadPool` is a `std::queue<std::function<void()>>`
(`ThreadPool.h:85`) — a task has no idea which worker is running it. Wiring this up needs, first:

```cpp
// ThreadPool.h
static uint32_t CurrentWorkerIndex();   // thread_local, set in the worker loop; UINT32_MAX off-pool
```

Then the adapter is straightforward — `enqueueTask` splits `[0, itemCount)` into
`ceil(itemCount / max(minRange, itemCount/workerCount))` chunks, enqueues one lambda per chunk,
returns a heap-allocated vector of futures as the opaque `void*`; `finishTask` `.get()`s them all
and frees it. No deadlock risk, because `finishTask` blocks the *main* thread, which is not a pool
worker.

Returning `nullptr` from `enqueueTask` is legal and tells Box2D you ran the work inline
(`types.h:36-37`) — a useful escape hatch and a good first implementation to validate the plumbing.

---

## 11. One world per scene

`Scene::MAX_SCENES` is 64 (`Scene.h:21`) and Box2D allows 128 non-interacting worlds. Given the
scene switching that just landed, the mapping is natural:

```cpp
std::array<b2WorldId, Scene::MAX_SCENES> worlds;
```

`CollisionSystem::Step` only steps `worlds[registry.GetActiveScene()]`. Benefits: a level's physics
state survives switching away and back; teardown is one `b2DestroyWorld` per scene; and the
`Registry::GetActiveSceneMask()` filtering that `ParallelForEach` does (`Registry.h:203`) becomes
unnecessary in the physics path, because a world only ever contains one scene's bodies.

If you'd rather not do this yet, the single-world version works — just make sure
`CollisionSystem::DeInit`/scene-switch destroys and rebuilds, or bodies from the old level will keep
colliding invisibly.

---

## 12. Fix these in the current code first

`Core/Source/Physics/CollisionSystem.cpp` has three problems that should go before any of the above
lands:

1. **`static_cast<b2Vec2>(0.0f, -10.0f)` is not a vector construction** (lines 20, 25, 35). The
   parenthesised expression is the comma operator, so this reads as
   `static_cast<b2Vec2>(-10.0f)` — casting a `float` to an aggregate with no converting
   constructor. Correct form:
   ```cpp
   worldDef.gravity = {0.0f, -10.0f};      // or b2Vec2{0.0f, -10.0f}
   ```
2. **The gravity value itself.** Should be `{0.0f, 0.0f}` for the current top-down camera (§2).
3. **`Init()` hardcodes a ground box and a falling box** (lines 24-42) — the Hello World sample from
   the docs. Fine as a smoke test, but it has to come out before entity-driven creation goes in, or
   you'll have invisible geometry in every scene.

Also: `CollisionSystem.h` includes `box2d/box2d.h` at line 4. That is fine — it's under
`Core/Source`, which `App` cannot reach. Just make sure that when `PhysicsSimulation.h` or anything
else moves toward `Core/Include`, the `b2WorldId` member goes behind a `unique_ptr` + forward
declaration, per the rules in `CLAUDE.md`'s "Vendor visibility". Right now `PhysicsSimulation.h`
includes `CollisionSystem.h` by value-ish (`unique_ptr`, but the include is there), so the Box2D
header is transitively in every TU that includes it — worth trimming to a forward declaration of
`CollisionSystem` while you're in there.

---

## 13. What this looks like from `App`

The goal, expressed as the code `Level.cpp` should be able to write:

```cpp
// A wall.
Entity wall = Entities::Create();
Entities::AddComponent<Transform>(wall, {.pos_x = -4.0f, .pos_y = 0.0f});
Entities::AddComponent<RigidBody>(wall, {.type = BodyType::Static});
Entities::AddComponent<Collider>(wall, {.half_width = 0.5f, .half_height = 0.5f});

// A crate that falls and tumbles.
Entity crate = Entities::Create();
Entities::AddComponent<Transform>(crate, {.pos_y = 4.0f});
Entities::AddComponent<RigidBody>(crate, {.type = BodyType::Dynamic});
Entities::AddComponent<Collider>(crate, {.friction = 0.3f});

// A pickup trigger.
Entity coin = Entities::Create();
Entities::AddComponent<Transform>(coin, {.pos_x = 2.0f});
Entities::AddComponent<RigidBody>(coin, {.type = BodyType::Static});
Entities::AddComponent<Collider>(coin, {.shape = ColliderShape::Circle,
                                        .half_width = 0.3f,
                                        .is_sensor = true});
```

Three `AddComponent` calls, no init call, no handle to keep, no `b2` type in sight. The body appears
on the next `PhysicsSimulation::Update` and `Transform` starts being driven for you.

For the imperative half, add `Core/Include/Api/Physics.h` alongside the existing `Api/` facades
(`Entities.h`, `Events.h`, `Images.h`, …), all of it in plain floats:

```cpp
namespace Mupfel::Physics
{
void SetVelocity(Entity e, float vx, float vy);
void ApplyImpulse(Entity e, float ix, float iy);
void ApplyForce(Entity e, float fx, float fy);
void Teleport(Entity e, float x, float y);   // b2Body_SetTransform + write Transform
std::pair<float, float> GetVelocity(Entity e);

/** Nearest shape hit along a ray, or nullopt. Wraps b2World_CastRayClosest. */
std::optional<RayHit> Raycast(float x0, float y0, float x1, float y1);
} // namespace Mupfel::Physics
```

These forward to `Application`'s `PhysicsSimulation` the way `Entities::` forwards to
`GetCurrentRegistry()`. Keeping them out of the components is what preserves the §3 ownership rule:
there is no way to write a velocity into a component and wonder why nothing happened.

---

## 14. Suggested order of work

1. Fix the three issues in §12. Get a single hardcoded dynamic box falling and *rendering* by
   hand-writing its `Transform` from `b2Body_GetPosition` — proves the unit/axis mapping (§2) before
   any architecture exists.
2. Flesh out `Collider`, add `RigidBody`, add `Registry::EntityFromIndex`, add the `bodies` side
   table, and implement `CreateBody` (§4, §5). Drive it from a one-off scan in `Init`.
3. Replace the hand-written readback with `b2World_GetBodyEvents` + `userData` (§7). This is the
   step that makes it fast.
4. Add the fixed-timestep accumulator (§7) and the create/destroy queues driven by
   `ComponentAddedEvent`/`EntityDestroyedEvent` (§6). Now entities are fully dynamic.
5. Contact and sensor events → `EventSystem` (§8). Migrate `Player` off direct `Transform` writes
   (§9).
6. Add `Api/Physics.h` (§13) and register a real `EntityFileManager::LoadCollider`
   (`EntityFileManager.cpp:80` is currently empty) so colliders survive serialization.
7. Per-scene worlds (§11).
8. Only if profiling says so: `ThreadPool` worker indices and the Box2D task callbacks (§10).

Steps 1–4 give you a working, fast integration. Everything after that is polish.
