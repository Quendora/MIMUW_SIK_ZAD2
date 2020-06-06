#include <csignal>

#include "outputManager.h"
#include "serverManager.h"

static int finish = false;

void handleSigInt(__attribute__((unused)) int s) {
    finish = true;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handleSigInt);

    std::shared_ptr<ParserServer> parser = std::make_shared<ParserServer>();
    if (parser->parseInput(argc, argv) == -1) fatal("Wrong parameters");

    std::shared_ptr<ShoutcastManager> shoutcast = std::make_shared<ShoutcastManager>();
    shoutcast->makeConnection(parser);

    if (!parser->checkIfFlagExists(PORT_FLAG_B)) {
        std::shared_ptr<OutputManager> output = std::make_shared<OutputManager>();
        output->sendStreamToStdout(stoi(parser->getValueOfFlag(TIMEOUT_FLAG_A)),
            shoutcast, &finish);
        shoutcast->closeSocket();
    } else {
        std::shared_ptr<ServerManager> server = std::make_shared<ServerManager>(parser, shoutcast);
        server->handleClients(&finish);
    }

    return 0;
}
