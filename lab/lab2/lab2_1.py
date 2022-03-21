#!/usr/bin/python                                                                            
                                                                                             
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.link import TCLink

class SingleSwitchTopo(Topo):
    "Single switch connected to n hosts."
    def build(self, n=2):
        switch = self.addSwitch('s1')
        # Python's range(N) generates 0..N-1
        for h in range(n):
            host = self.addHost('h%s' % (h + 1))
            self.addLink(host, switch)

class Topo1(Topo):
    topo_edges = [{'n0': 's1', 'n1': 'h2'},
                  {'n0': 's1', 'n1': 'h4'},
                  {'n0': 's1', 'n1': 's2','bw': 10,'loss': 0},
                  {'n0': 's1', 'n1': 's3','bw': 10,'loss': 0},
                  {'n0': 's2', 'n1': 'h6'},
                  {'n0': 's2', 'n1': 'h5'},
                  {'n0': 's3', 'n1': 'h1'},
                  {'n0': 's3', 'n1': 'h3'}
                  ]
 
    def build(self):
        for edge in self.topo_edges:
            n0, n1, bw, loss= edge.get('n0'), edge.get('n1'), edge.get('bw'), edge.get('loss')
            if 's' in n0:
                node0 = self.addSwitch(n0)
            if 's' in n1:
                node1 = self.addSwitch(n1)
            if 'h' in n0:
                node0 = self.addHost(n0)
            if 'h' in n1:
                node1 = self.addHost(n1)

            
            if bw and loss:
                self.addLink(n0, n1, bw=bw, loss=loss)
            elif bw:
                self.addLink(n0, n1, bw=bw)
            else:
                self.addLink(n0, n1)
            

            



def simpleTest():
    "Create and test a simple network"
    topo = SingleSwitchTopo(n=4)
    net = Mininet(topo)
    net.start()
    print( "Dumping host connections" )
    dumpNodeConnections(net.hosts)
    print( "Testing network connectivity" )
    net.pingAll()
    net.stop()

def my_test():
    print("starting...")
    topo = Topo1()
    net = Mininet(topo=topo, link=TCLink)
    net.start()
    print( "Dumping host connections" )
    dumpNodeConnections(net.hosts)
    print( "Testing network connectivity")
    net.pingAll()
    
    h1 = net.get('h1')
    for i in range(2,7):
        hi = net.get(f'h{i}')
        net.iperf((h1,hi))

    net.stop()

if __name__ == '__main__':
    # Tell mininet to print useful information
    
    setLogLevel('info')
    my_test()