This Max External handles the logic for a Max For Live Device I created for a client called Midi Declutter. Max for Live devices are third party plugins that run in Ableton Live, a Digital Audio Workstation. 

Midi Declutter filters incoming midi notes depending on their relationship to notes currently being played on the devices assigned midi track (called local pool) and notes being played on other instances of the device on other midi tracks (called the global pool). Incoming notes creating intervals smaller than a minor 3rd with any note in the local pool are dropped and notes meeting that same criteria with the global pool are altered to a minor third of the first encountered note in the pool. If that altered noted creates other "conflicts" 3 more attempts will be made to alter the new pitch value. If a conflict still exists the note is dropped. 

I'm also working on a second version for the client with seperate logic that handles the resolution of "conflicting" intervals in another manner. I will include that code here as a seperate branch soon. 

