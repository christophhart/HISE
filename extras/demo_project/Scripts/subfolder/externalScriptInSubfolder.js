/** External script
*
*	You can also write external scripts and include them into your scripts via `include("filename.js");`.
*
*	They will be parsed when the script is loaded. This is useful for boring data definitions or common API enhancements.
*/

Console.print("This external script is also compiled");

externalInSubfolder = Content.addLabel("externalInSubfolder", 0, 50);
externalInSubfolder.set("saveInPreset", false);