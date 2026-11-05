# Decisions
## PreCompiler
### Un auto-deletable variables:
[//]: # (2026. 10. 06. 16:01)
- if left undeleted, they will yield a warning and get automatically deleted at the end of the scope, instead of the last known usage
    - if the user marks a variable as `used` they probably have a good reason for it
    - if the user forgets to delete it
        - tell them, so they can fix it, because it's probably a bug on their part
        - don't halt precompilation over it, just play it safe and delete it at its last **possible** usage - the end of the scope - instead of the **known** usage, just to be sure