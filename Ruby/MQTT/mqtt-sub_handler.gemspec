Gem::Specification.new do |s|
	s.name        = 'mqtt-sub_handler'
	s.version     = '0.1.0'
	s.date        = '2018-06-26'
	s.summary     = "Asynchronous, topic-based MQTT gem"
	s.description = "Asynchronous handling of callbacks that can be attached to individual topics, based on the mqtt gem."
	s.authors     = ["Xasin"]
	s.files       = [	"lib/mqtt/sub_handler.rb",
							"lib/mqtt/sub_testing.rb",
							"lib/mqtt/subscription_classes.rb",
							"lib/mqtt/Waitpoint.rb",
							"README.md"]
	s.homepage    =
	'https://github.com/XasWorks/XasCode/tree/MQTT_GEM/Ruby/MQTT'
	s.license     = 'GPL-3.0'

	s.add_runtime_dependency "mqtt", ">= 0.5.0"
	s.add_runtime_dependency "json"

	s.add_development_dependency "minitest"
	s.add_development_dependency "guard"
	s.add_development_dependency "guard-minitest"
end