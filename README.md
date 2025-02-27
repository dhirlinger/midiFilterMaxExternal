This Max External handles the logic for a Max For Live Device I created for a client called Midi Declutter. Max for Live devices are third party plugins that run in Ableton Live, a Digital Audio Workstation. This is my first Max External and first adventure in C++. I used the [Max-SDK](https://github.com/Cycling74/max-sdk) to make it happen. 

 Midi Declutter filters incoming midi notes depending on their relationship to notes currently being played on the device's assigned midi track (called local pool) and notes being played on other instances of the device on other midi tracks (called the global pool). There are 2 versions of the device. Incoming notes creating intervals smaller than a minor 3rd with any note in the local pool are dropped and notes meeting that same criteria with the global pool are altered to a minor third of the first encountered note in the global pool in version 1 and moved to an existing conflicting note in version 2. Per the client's request conflicts below the incoming note are checked first and moved to that pitch before checking for conflicts above. If that altered note creates other "conflicts" 3 more attempts will be made to alter the new pitch value. If a conflict still exists the note is dropped. 

The main branch correspondes to version 1 and version-2 branch to version 2.  

