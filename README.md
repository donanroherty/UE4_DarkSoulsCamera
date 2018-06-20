# Dark Souls Camera

## About

This is a fairly accurate recreation of a Dark Souls style camera system in Unreal Engine 4.18. It supports locking to targets, scrolling between available targets, camera relative character movement and an optional soft-lock system that automatically manages locking to closest target in range. Project is built in C++ with some blueprint logic for anim trees.

## Controls

| Input                | Mouse/Keyboard                    | Gamepad                |
| -------------------- | :-------------------------------- | ---------------------- |
| **Movement**         | WASD                              | Left Analog Stick      |
| **Aiming**           | Mouse Right                       | Right Analog Button    |
| **Toggle Hard Lock** | Middle Mouse Button               | Right Analog Button    |
| **Toggle Soft Lock** | Enter                             | Left Analog Button     |
| **Switch Target**    | Mouse Left/Right (Small Movement) | Right Stick Left/Right |
| **Break Soft Lock**  | Mouse Left/Right (Fast movement)  |                        |
| **Toggle Debug**     | Tab                               | Start                  |

## Overview

### Lock on

When locking on, the controller’s rotation is aligned to point at the target. Rotation lag is enabled on the camera spring arm for smooth movement.
An overlap test centered on the player finds all DSTargetComponents within range and chooses the one closest to center screen.

### Hard lock

Hard-lock is the same as Dark Souls and the default in this project. Player presses lock button, sphere overlap test finds targets in range and camera locks to the target most central to the players view.

### Soft lock

Soft-lock, when enabled, locks onto a nearby target in range and breaks lock when no targets are in range.  
Soft-lock can also be broken with a harsh mouse movement. This disables the soft-lock system until the player leaves and re-enters the enemy’s range, disables/re-enables soft-lock or by pressing the hard lock button (R3). This resets bool bSoftlockRequiresReset back to false.
I tried to keep to the brief but the brief doesn’t state when to reenable soft-lock so I went with this method of requiring lock to prevent soft-lock re-acquiring a target immediately after breaking lock.

### Target switching

Target switching attempts to find a new target in the direction of the players input. It uses the cross–product of the current target vector and a new target vector to filter targets based on direction, then compares dot-products to get the target with the closest angle to the existing target.
Target switching is automatic when using soft-lock and the original target leaves lock-on range.

### DSTargetComponent

Adding this component to an actor makes it targetable. Actors can have multiple targets allowing for large enemies with multiple target points. DSTargetComponent extends USphereComponent and is detected using a component overlap test.

### Improvements

- Camera should lock to target at an angle, placing enemy in the upper portion of the screen rather than dead center hiding the enemy behind the player model.

- Currently when locked on the player moves perpendicular to a vector pointed at the target. Over time the players strafe circle increases. This could be improved by moving the player toward a point on a circle around the target.

- System handles targets that move out of range gracefully, but not targets that have died or are otherwise made unviable.
