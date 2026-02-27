#include "StarterBot.h"
#include "Tools.h"
#include "MapTools.h"

StarterBot::StarterBot()
{
    
}

// Called when the bot starts!
void StarterBot::onStart()
{
    // Set our BWAPI options here    
	BWAPI::Broodwar->setLocalSpeed(10);
    BWAPI::Broodwar->setFrameSkip(0);
    
    // Enable the flag that tells BWAPI top let users enter input while bot plays
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);

    // Call MapTools OnStart
    m_mapTools.onStart();
}

// Called on each frame of the game
void StarterBot::onFrame()
{
    // Update our MapTools information
    m_mapTools.onFrame();

    // Execute the mission
    executeMission();

    // Send our idle workers to mine minerals so they don't just stand there
    sendIdleWorkersToMinerals();

    // Train more workers so we can gather more income
    trainAdditionalWorkers();

    // Build more supply if we are going to run out soon
    buildAdditionalSupply();

    // Draw unit health bars, which brood war unfortunately does not do
    Tools::DrawUnitHealthBars();

    // Draw some relevent information to the screen to help us debug the bot
    drawDebugInformation();
}

// Send our idle workers to mine minerals so they don't just stand there
void StarterBot::sendIdleWorkersToMinerals()
{
    // Let's send all of our starting workers to the closest mineral to them
    // First we need to loop over all of the units that we (BWAPI::Broodwar->self()) own
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    for (auto& unit : myUnits)
    {
        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if (unit->getType().isWorker() && unit->isIdle())
        {
            // Get the closest mineral to this worker unit
            BWAPI::Unit closestMineral = Tools::GetClosestUnitTo(unit, BWAPI::Broodwar->getMinerals());

            // If a valid mineral was found, right click it with the unit in order to start harvesting
            if (closestMineral) { unit->rightClick(closestMineral); }
        }
    }
}

// Train more workers so we can gather more income
void StarterBot::trainAdditionalWorkers()
{
    const BWAPI::UnitType workerType = BWAPI::Broodwar->self()->getRace().getWorker();
    const int workersWanted = 20;
    const int workersOwned = Tools::CountUnitsOfType(workerType, BWAPI::Broodwar->self()->getUnits());
    if (workersOwned < workersWanted)
    {
        // get the unit pointer to my depot
        const BWAPI::Unit myDepot = Tools::GetDepot();

        // if we have a valid depot unit and it's currently not training something, train a worker
        // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
        if (myDepot && !myDepot->isTraining()) { myDepot->train(workerType); }
    }
}

// Build more supply if we are going to run out soon
void StarterBot::buildAdditionalSupply()
{
    // Get the amount of supply supply we currently have unused
    const int unusedSupply = Tools::GetTotalSupply(true) - BWAPI::Broodwar->self()->supplyUsed();

    // If we have a sufficient amount of supply, we don't need to do anything
    if (unusedSupply >= 2) { return; }

    // Otherwise, we are going to build a supply provider
    const BWAPI::UnitType supplyProviderType = BWAPI::Broodwar->self()->getRace().getSupplyProvider();

    const bool startedBuilding = Tools::BuildBuilding(supplyProviderType);
    if (startedBuilding)
    {
        BWAPI::Broodwar->printf("Started Building %s", supplyProviderType.getName().c_str());
    }
}

// Draw some relevent information to the screen to help us debug the bot
void StarterBot::drawDebugInformation()
{
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 10), "Hello, World!\n");
    Tools::DrawUnitCommands();
    Tools::DrawUnitBoundingBoxes();
}

// Execute the mission: Cast hallucinations, load shuttle, and move to destination
void StarterBot::executeMission()
{
    if (!missionStarted)
    {
        missionStarted = true;
        BWAPI::Broodwar->printf("Mission started!");
    }

    // Step 1: Cast hallucinations on the shuttle
    if (!hallucination1Cast || !hallucination2Cast)
    {
        castHallucinations();
        return;
    }

    // Step 2: Load units into the shuttle
    if (!unitsCargaed)
    {
        loadShuttle();
        return;
    }

    // Step 3: Move to destination
    moveToDestination();
}

// Cast Hallucination 2 times on the shuttle
void StarterBot::castHallucinations()
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    BWAPI::Unit tassadar = nullptr;
    BWAPI::Unit shuttle = nullptr;

    // Find Tassadar and Shuttle
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Hero_Tassadar)
        {
            tassadar = unit;
        }
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle)
        {
            shuttle = unit;
        }
    }

    if (!tassadar || !shuttle)
    {
        BWAPI::Broodwar->printf("Tassadar or Shuttle not found!");
        return;
    }

    // Cast first hallucination
    if (!hallucination1Cast)
    {
        tassadar->useTech(BWAPI::TechTypes::Hallucination, shuttle);
        hallucination1Cast = true;
        BWAPI::Broodwar->printf("First Hallucination cast!");
    }
    // Cast second hallucination (wait a few frames)
    else if (!hallucination2Cast && BWAPI::Broodwar->getFrameCount() % 20 == 0)
    {
        tassadar->useTech(BWAPI::TechTypes::Hallucination, shuttle);
        hallucination2Cast = true;
        BWAPI::Broodwar->printf("Second Hallucination cast! Total 4 hallucinations created.");
    }
}

// Load Tassadar and 2 Zealots into the real shuttle
void StarterBot::loadShuttle()
{
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    BWAPI::Unit realShuttle = nullptr;
    BWAPI::Unit tassadar = nullptr;
    int zealotsLoaded = 0;

    // Find the real shuttle (not a hallucination) and Tassadar
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle && !unit->isHallucination())
        {
            realShuttle = unit;
        }
        if (unit->getType() == BWAPI::UnitTypes::Hero_Tassadar)
        {
            tassadar = unit;
        }
    }

    if (!realShuttle)
    {
        BWAPI::Broodwar->printf("Real shuttle not found!");
        return;
    }

    // Load Tassadar
    if (tassadar && !tassadar->isLoaded())
    {
        realShuttle->load(tassadar);
        BWAPI::Broodwar->printf("Loading Tassadar...");
    }

    // Load 2 Zealots
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && zealotsLoaded < 2)
        {
            if (!unit->isLoaded())
            {
                realShuttle->load(unit);
                zealotsLoaded++;
                BWAPI::Broodwar->printf("Loading Zealot %d...", zealotsLoaded);
            }
        }
    }

    // Check if shuttle is loaded with all units
    int loadedCount = 0;
    if (tassadar && tassadar->isLoaded()) loadedCount++;
    
    for (auto& unit : myUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot && unit->isLoaded())
        {
            loadedCount++;
        }
    }

    if (loadedCount >= 3)  // Tassadar + 2 Zealots
    {
        unitsCargaed = true;
        BWAPI::Broodwar->printf("Shuttle loaded successfully!");
    }
}

// Move the real shuttle and all hallucinations to the destination
void StarterBot::moveToDestination()
{
    const BWAPI::Position destination(3472, 3488);
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();

    for (auto& unit : myUnits)
    {
        // Move all shuttles (real and hallucinations) to destination
        if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle)
        {
            unit->move(destination);
        }
    }
}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner)
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
}

// Called whenever a unit is destroyed, with a pointer to the unit
void StarterBot::onUnitDestroy(BWAPI::Unit unit)
{
	
}

// Called whenever a unit is morphed, with a pointer to the unit
// Zerg units morph when they turn into other units
void StarterBot::onUnitMorph(BWAPI::Unit unit)
{
	
}

// Called whenever a text is sent to the game by a user
void StarterBot::onSendText(std::string text) 
{ 
    if (text == "/map")
    {
        m_mapTools.toggleDraw();
    }
}

// Called whenever a unit is created, with a pointer to the destroyed unit
// Units are created in buildings like barracks before they are visible, 
// so this will trigger when you issue the build command for most units
void StarterBot::onUnitCreate(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit finished construction, with a pointer to the unit
void StarterBot::onUnitComplete(BWAPI::Unit unit)
{
	
}

// Called whenever a unit appears, with a pointer to the destroyed unit
// This is usually triggered when units appear from fog of war and become visible
void StarterBot::onUnitShow(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit gets hidden, with a pointer to the destroyed unit
// This is usually triggered when units enter the fog of war and are no longer visible
void StarterBot::onUnitHide(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit switches player control
// This usually happens when a dark archon takes control of a unit
void StarterBot::onUnitRenegade(BWAPI::Unit unit)
{ 
	
}