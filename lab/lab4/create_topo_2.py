#!/usr/bin/python                                                                            
                                                                                             
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.link import TCLink



class Topo4(Topo):

    def build(self):
        s2 = self.addSwitch('s2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        self.addLink(s2, h3, bw=10, loss=0, delay='5ms')
        self.addLink(s2, h4, bw=10, loss=0, delay='5ms')


def my_test():
    print("starting...")
    topo = Topo4()
    net = Mininet(topo=topo, link=TCLink)
    net.start()
    # print( "Dumping host connections" )
    # dumpNodeConnections(net.hosts)
    
    # net.startTerms()
    CLI(net)
    net.stop()

if __name__ == '__main__':
    # Tell mininet to print useful information
    setLogLevel('info')
    my_test()