#include "clientManager.h"

static int finish = false;

void handleSigInt(__attribute__((unused)) int s) {
    finish = true;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handleSigInt);
    signal(SIGPIPE, SIG_IGN);

    std::shared_ptr<ParserClient> parser = std::make_shared<ParserClient>();
    if (parser->parseInput(argc, argv) == -1) fatal("Wrong parameters");

    std::shared_ptr<ClientManager> client = std::make_shared<ClientManager>(parser);
    client->handleConnections(&finish);

    return 0;
}