echo "adding flow rules.."
sudo ovs-ofctl add-flow s1 "in_port=3 actions=output:1,2"
sudo ovs-ofctl add-flow s1 "in_port=4 actions=output:1,2"
