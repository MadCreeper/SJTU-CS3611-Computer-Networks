#!/usr/bin/python                                                                            
                                                                                             
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.link import TCLink



class Topo2(Topo):
    topo_edges = [{'n0': 's1', 'n1': 'h2'},
                  {'n0': 's1', 'n1': 'h4'},
                  {'n0': 's1', 'n1': 's2','bw': 10,'loss': 10}, # 10Mbps, 10% packet loss
                  {'n0': 's1', 'n1': 's3','bw': 10,'loss': 10}, # 10Mbps, 10% packet loss
                  {'n0': 's2', 'n1': 's3'}, # new edge
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


def my_test():
    print("starting...")
    topo = Topo2()
    net = Mininet(topo=topo, link=TCLink)
    net.start()
    print( "Dumping host connections" )
    dumpNodeConnections(net.hosts)
    #print( "Testing network connectivity")
    #net.pingAll()
    
    """
    h1 = net.get('h1')
    for i in range(2,7):
        hi = net.get(f'h{i}')
        net.iperf((h1,hi))
    """
   
    CLI(net)
    net.stop()

if __name__ == '__main__':
    # Tell mininet to print useful information
    setLogLevel('info')
    my_test()