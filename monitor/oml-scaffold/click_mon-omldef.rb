
defApplication('root:app:click_mon', 'click_mon') do |app|

  app.version(0, 9, 1)
  app.shortDescription = 'MF Click router performance monitor'
  app.description = %{
	Monitors MobilityFirst's Click-based router performance
	by periodically querying Click's control interface for
	pre-defined performance stats, and injects these stats
	for archival at the OML server.
  }
  app.path = "/usr/local/bin/mf_click_mon"

  app.defMeasurement("packet_stats") do |mp|
    mp.defMetric("mp_index", "uint32")
    mp.defMetric("node_id", "string")
    mp.defMetric("port_id", "string")
    mp.defMetric("in_pkts", "uint64")
    mp.defMetric("out_pkts", "uint64")
    mp.defMetric("errors", "uint64")
    mp.defMetric("dropped", "uint64")
    mp.defMetric("in_bytes", "uint64")
    mp.defMetric("out_bytes", "uint64")
    mp.defMetric("in_tput_mbps", "double") 
    mp.defMetric("out_tput_mbps", "double") 
  end

  app.defMeasurement("link_stats") do |mp|
    mp.defMetric("mp_index", "uint32")
    mp.defMetric("link_label", "string") 
    mp.defMetric("node_id", "string")
    mp.defMetric("nbr_id", "string") 
    mp.defMetric("bitrate_mbps", "double") 
    mp.defMetric("s_ett_usec", "uint32") 
    mp.defMetric("l_ett_usec", "uint32") 
    mp.defMetric("in_pkts", "uint64") 
    mp.defMetric("out_pkts", "uint64") 
    mp.defMetric("in_bytes", "uint64") 
    mp.defMetric("out_bytes", "uint64") 
    mp.defMetric("in_tput_mbps", "double") 
    mp.defMetric("out_tput_mbps", "double") 
  end

  app.defMeasurement("routing_stats") do |mp|
    mp.defMetric("mp_index", "uint32")
    mp.defMetric("node_id", "string")
    mp.defMetric("in_chunks", "uint64")
    mp.defMetric("out_chunks", "uint64")
    mp.defMetric("in_ctrl_msgs", "uint64")
    mp.defMetric("out_ctrl_msgs", "uint64")
    mp.defMetric("stored_chunks", "uint64")
    mp.defMetric("error_chunks", "uint64")
    mp.defMetric("dropped_chunks", "uint64")
    mp.defMetric("in_data_bytes", "uint64")
    mp.defMetric("out_data_bytes", "uint64")
    mp.defMetric("in_ctrl_bytes", "uint64")
    mp.defMetric("out_ctrl_bytes", "uint64")
  end

end

# Local Variables:
# mode:ruby
# End:
# vim: ft=ruby:sw=2
