/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGInput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file or stdout
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGInput.h"
#include "FGState.h"
#include "FGFDMExec.h"

#include <fstream>
#include <iomanip>

namespace JSBSim {

static const char *IdSrc = "$Id: FGInput.cpp,v 1.2 2005/07/24 21:00:34 jberndt Exp $";
static const char *IdHdr = ID_INPUT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGInput::FGInput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGInput";
  sFirstPass = dFirstPass = true;
  socket = 0;
  port = 0;
  enabled = true;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInput::~FGInput()
{
  if (socket) delete socket;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::Run(void)
{
  string line, token;
  int start=0, string_start=0, string_end=0;
  int token_start=0, token_end=0;
  char buf[100];
  double value=0;
  FGPropertyManager* node=0;

  if (FGModel::Run()) return true; // fast exit if nothing to do
  // This model DOES execute if "Exec->Holding"

  data = socket->Receive(); // get socket transmission if present

  if (data.size() > 0) {
    // parse lines
    while (1) {
      string_start = data.find_first_not_of("\r\n", start);
      if (string_start == string::npos) break;
      string_end = data.find_first_of("\r\n", string_start);
      if (string_end == string::npos) break;
      line = data.substr(string_start, string_end-string_start);
      if (line.size() == 0) break;

      // now parse individual line
      token_start = line.find_first_not_of(" ", 0);
      token_end = line.find_first_of(" ", token_start);
      token = line.substr(token_start, token_end - token_start);
      if (token == "set" || token == "SET" ) {                   // SET PROPERTY
        token_start = line.find_first_not_of(" ", token_end);
        token_end = line.find_first_of(" ", token_start);
        token = line.substr(token_start, token_end-token_start);
        node = PropertyManager->GetNode(token);
        if (node == 0) socket->Reply("Unknown property\n");
        else {
          token_start = line.find_first_not_of(" ", token_end);
          token_end = line.find_first_of(" ", token_start);
          token = line.substr(token_start, token_end-token_start);
          value = atof(token.c_str());
          node->setDoubleValue(value);
        }
      } else if (token == "get" || token == "GET") {             // GET PROPERTY
        token_start = line.find_first_not_of(" ", token_end);
        token = line.substr(token_start, line.size()-token_start);
        node = PropertyManager->GetNode(token);
        if (node == 0) socket->Reply("Unknown property\n");
        else {
          sprintf(buf, "%s = %12.6f\n", token.c_str(), node->getDoubleValue());
          socket->Reply(buf);
        }
      } else if (token == "hold" || token == "HOLD") {                // PAUSE
        FDMExec->Hold();
      } else if (token == "resume" || token == "RESUME") {             // RESUME
        FDMExec->Resume();
      } else if (token == "quit" || token == "QUIT") {                   // QUIT
        // close the socket connection
        socket->Close();
      } else if (token == "info" || token == "INFO") {                   // INFO
        // get info about the sim run and/or aircraft, etc.
      } else if (token == "help" || token == "HELP") {                   // HELP
        socket->Reply(
        " JSBSim Server commands:\n\n"
        "   get {property name}\n"
        "   set {property name} {value}\n"
        "   hold\n"
        "   resume\n"
        "   help\n"
        "   quit\n"
        "   info\n\n");
      } else {
        socket->Reply(string("Unknown command: ") +  token + string("\n"));
      }

      start = string_end;
    }
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::Load(Element* element)
{
  string type="", parameter="";
  string name="", fname="";
  string property;

  port = element->GetAttributeValueAsNumber("port");
  if (port == 0) {
    cerr << endl << "No port assigned in input element" << endl;
  } else {
    socket = new FGfdmSocket(port);
  }

  Debug(2);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGInput::Debug(int from)
{
  string scratch="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
    if (from == 2) {
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGInput" << endl;
    if (from == 1) cout << "Destroyed:    FGInput" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}