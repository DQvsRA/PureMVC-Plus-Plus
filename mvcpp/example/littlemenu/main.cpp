/**
 *	Application entry point for littlemenu.
 *	This is a tiny application that displays a menu using
 *	the C++ version of the PureMVC framework.
 *
 *	author Schell Scivally
 *	since 15.12.2009
 */
#include <iostream>
#include <sstream>
#include <string>
#include "pmvcpp.h"

using namespace std;

//--------------------------------------
//  Notification Definitions
//--------------------------------------
// let's enumerate our notification names and types
class n_name
{
public:
    enum name
    {
        STARTUP,                // triggers the app startup sequence
        PATTERNS_REGISTERED,	// alerts the app that proxies and mediators have been registered
        SET,			// sets something
        GET,			// makes a request to get something
        DISPLAY,		// display something
        QUIT                    // quit the app
	};
	vector<char*> toString;
};
class n_type
{
public:
    enum type
    {
        MENU    // the menu
	};
	static const map<int, char*> toString;
};
// list string conversions for pretty debug printing

n_type::toString[MENU] = "menu";
//--------------------------------------
//  Notification Body Definitions
//--------------------------------------
class MenuNotificationBody : public IBody
{
public:	
	vector<string> data;
	
	MenuNotificationBody(vector<string> menu) : data(menu) {}
	string getType()
	{
		return "MenuNotificationBody";
	}
};
//--------------------------------------
//  Proxies
//--------------------------------------
/**
 *	MenuProxy - holds the choices for our menu.
 */
class MenuProxy : public Proxy<vector<string> >
{
public:
	static const string NAME;
	
	MenuProxy(string proxyName) : Proxy<vector<string> >(proxyName) {}
	void onRegister()
	{
		cout << "MenuProxy::onRegister()\n";
		this->setData(this->getMenu());
	}
	void onRemove()
	{
		cout << "MenuProxy::onRemove()\n";
	}
	vector<string> getMenu()
	{
		cout << "MenuProxy::getMenu()\n";
		vector<string> menu;
		menu.push_back("Fox");
		menu.push_back("Bear");
		menu.push_back("Mountain Lion");
		menu.push_back("Lynx");
		menu.push_back("Bobcat");
		menu.push_back("Terradactyl");
		menu.push_back("Wolf");
		menu.push_back("Other...");
		return menu;
	}
};
// define the proxy's name
const string MenuProxy::NAME = "MenuProxy";
//--------------------------------------
//  Mediators
//--------------------------------------
/**
 *	CLIMediator - mediates interaction between the terminal and the user.
 */
class CLIMediator : public Mediator<bool>
{
public:
	static const string NAME;
	
	CLIMediator(string mediatorName, bool viewComponent) : Mediator<bool>(mediatorName, viewComponent) {}
	void onRegister()
	{
		cout << "CLIMediator::onRegister()\n";
	}
	void onRemove()
	{
		cout << "CLIMediator::onRemove()\n";
	}
	// list all of the notifications we're interested in
	vector<int> listNotificationInterests()
	{
		vector<int> interests;
		
		interests.push_back(n_name::PATTERNS_REGISTERED);
		interests.push_back(n_name::DISPLAY);
		
		return interests;
	}
	// deal with interesting notifications
	void handleNotification(INotification* note)
	{
		cout << "CLIMediator::handleNotification(name:" << note->getName() << " type:" << note->getType() <<")\n";
		
		string name = note->getName();
		string type = note->getType();
		IBody* body = note->getBody();
		// handle the notification
		// if the app is ready, ask for the menu
		if(name == n_name::PATTERNS_REGISTERED)
			this->sendNotification(n_name::GET, n_type::MENU);
		else if(name == n_name::DISPLAY)
		{
			if(type == n_name::MENU)
			{
				cout << "	menu received...\n";
				this->promptUser(dynamic_cast<MenuNotificationBody*>(body)->data);
			}
		}
	}
	void promptUser(vector<string> menu)
	{
		size_t choices = menu.size();
		int choice = 0;
		string input = "";
		while(choice != (int) choices)
		{
			cout << "\n- What is your favorite forest creature? -\n";
			for(size_t i = 1; i <= choices; ++i)
			{
				cout << "	" << i << ". " << menu.at(i-1) << "\n";
			}
			cout << "[1 - " << choices << " or 'q']:";
			while(true)
			{
				getline(cin, input);
				
				if(input == "q")
					return this->quit();
					
				stringstream myStream(input);
				if(myStream >> choice && choice <= (int) choices && choice > 0)
					break;
				cout << "- Wait, what? -\n[1 - " << choices << " or 'q']:";
			}
			if(choice < (int) choices)
				cout << "\n- A " << menu.at(choice - 1) << " is pretty. -\n";
		}
		cout << "\n- You like some other animal? -\nAdd your favorite animal [some animal]:";
		getline(cin, input);
		cout << "\n";
		
		// add the new animal to the list
		string other = menu.at(choices - 1);
		// swap so "Other.." stays last
		menu.at(choices - 1) = input;
		menu.push_back(other);
		MenuNotificationBody* newMenu = new MenuNotificationBody(menu);
		// send it off to someone who needs it...
		this->sendNotification(n_name::SET, newMenu, n_type::MENU);
	}
	void quit()
	{
		// there's nothing keeping control besides this mediator, so this is enough to quit.
	}
};
// define the mediator's name
const string CLIMediator::NAME = "CLIMediator";
//--------------------------------------
//  Commands
//--------------------------------------
/**
 *	Startup - registers our proxies and mediators.
 */
class Startup : public SimpleCommand
{
public:
	void execute(INotification* note)
	{
		cout << "Startup::execute()\n";
		IFacade* facade = this->getFacade();
		// register mediators
		facade->registerMediator(new CLIMediator(CLIMediator::NAME, false));
		// register proxies
		facade->registerProxy(new MenuProxy(MenuProxy::NAME));
		
		// now let the app know we're done registering things...
		facade->sendNotification(n_name::PATTERNS_REGISTERED);
	}
};
/**
 *	Set - sets things.
 */
class Set : public SimpleCommand
{
public:
	void execute(INotification* note)
	{
		cout << "Set::execute(type:" << note->getType() << ")\n";
		
		IFacade* facade = this->getFacade();
		string type = note->getType();
		// handle the request
		if(type == n_name::MENU)
		{
			MenuNotificationBody* menuBody = dynamic_cast<MenuNotificationBody*>(note->getBody());
			vector<string> menu = menuBody->data;
			cout << "	adding " << menu.at(menu.size() - 1) << " to the menu...\n";
			// get the menu proxy
			MenuProxy* menuProxy = dynamic_cast<MenuProxy*>(facade->retrieveProxy(MenuProxy::NAME));
			menuProxy->setData(menu);
			menuBody->data = menuProxy->getData();
			// send the menu back out to be displayed
			facade->sendNotification(n_name::DISPLAY, menuBody, n_type::MENU);
		}
	}
};
/**
 *	Get - gets things.
 */
class Get : public SimpleCommand
{
public:
	void execute(INotification* note)
	{
		cout << "Get::execute(type:" << note->getType() << ")\n";
		
		IFacade* facade = this->getFacade();
		string type = note->getType();
		// handle the request
		if(type == n_name::MENU)
		{
			cout << "	requested the menu...\n";
			// get the menu proxy
			MenuProxy* menuProxy = dynamic_cast<MenuProxy*>(facade->retrieveProxy(MenuProxy::NAME));
			vector<string> menu = menuProxy->getData();
			// send the menu out to who needs it...
			facade->sendNotification(n_name::DISPLAY, new MenuNotificationBody(menu), n_name::MENU);
		}
	}
};
/**
 *	Register our commands.
 */
void registerCommands(Facade* facade)
{
	// register the command to respond to notification definitions
	facade->registerCommand<Startup>(n_name::STARTUP);
	facade->registerCommand<Set>(n_name::SET);
	facade->registerCommand<Get>(n_name::GET);
}
//--------------------------------------
//  MAIN
//--------------------------------------
int main(int argc, char** argv)
{
	cout << "\n-- littlemenu v0.1 by Schell Scivally --\n\n";
	// set some key for the app, preferably one that won't collide
	// with some other one down the road
	string applicationKey = "littlemenuApplicationKey123456";
	// create an instance of the facade
	Facade* facade = Facade::getInstance(applicationKey);
	// register our commands through the facade
	registerCommands(facade);
	// startup the app by calling the startup command
	facade->sendNotification(n_name::STARTUP);
	
	// cleanup the app for quitting
	Facade::removeCore(applicationKey);
	
	cout << "\n- bye!\n\n";
	return 0;
}