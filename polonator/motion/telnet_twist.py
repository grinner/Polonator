from twisted.internet import reactor
from twisted.internet.protocol import ClientFactory
from twisted.conch.telnet import Telnet,TelnetTransport, StatefulTelnetProtocol

"""
The Maestro communicates over Telnet.  This is a client connect to the Maestro's telnet server and send commands
"""

#class MaestroClientProtocol(Telnet):
class MaestroClientProtocol(StatefulTelnetProtocol):
    def connectionMade(self):
        self.mybuffer = ''
        print "connected!\n"
    def dataReceived(self, data):
        self.mybuffer += data;
        if data == ' ' or data == '\n':
            #if "MAESTRO" in self.mybuffer:
            if ">" in data:
                #self.sendLine(self.factory.message)
                print "gonna move a filter\n"
            print 'Received:', self.mybuffer
            self.mybuffer = ''
            
    def lineReceived(self, line):
        if "MAESTRO" in line:
            d = self.sendLine("quit\n")
        print line
    def connectionLost(self, reason):
        reactor.stop()
        print "done."
        
        
class MaestroFactory(ClientFactory):

    #protocol = MaestroClientProtocol
    #protocol = TelnetTransport
    def __init__(self, message):
        self.message = message
        print self.protocol
    # end def

    def buildProtocol(self, addr):
    	p = TelnetTransport(MaestroClientProtocol)
        p.factory = self
        return p
	# end def
# end class
        
        
if __name__ == '__main__':
    host = '10.0.0.56'
    port = 23
    #filter_number = 0 # cy3
    filter_number = 3 # cy5
    message =  "Theta.xq##gotofilter(%d);\n\r" % filter_number
    maestrof = MaestroFactory(message)
    reactor.connectTCP(host, port, maestrof)
    reactor.run()
