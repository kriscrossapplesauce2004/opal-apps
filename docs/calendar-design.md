# Calendar + Contacts — design decisions (round 1, 2026-10-08) — LOCKED ("Hell yeah")

Renders: `artifacts/calendar/`, QML: `prototypes/calendar/`.

- **Week** (default desktop view): left panel with New event FAB, mini month (today + shown week marked), calendar list with colour checkboxes, sync card (CalDAV source); week grid with tonal event blocks (calendar colour stripe, title, time · place), all-day row, red now line; Day / Week / Month / Agenda segmented switch, search.
- **Month**: month grid with event chips; side panel with the chosen day's agenda and birthdays.
- **Contacts** (a destination in the same app): searchable list with avatars and birthday marks; detail card with a split-pill action row (Call / Message / Email / Invite), fields, and "Together" (shared upcoming events).
- **Phone**: week strip that pulls down into a month, agenda cards, new-event bottom sheet with chips (date, time, calendar, reminder, invite).
- **Couch**: glanceable today (huge date, birthday pill) and the next events as big cards.
- Backend: CalDAV/CardDAV with a local store; Opal shell calendar widget reads from it.
