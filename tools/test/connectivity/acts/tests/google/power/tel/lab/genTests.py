
for b in [4, 7, 13]:
    if b == 4:
        bwl = [1.4, 3, 5, 10, 20]
    elif b == 7:
        bwl = [5, 10, 20]
    elif b == 13:
        bwl = [5, 10]
        
    for bw in bwl:
        
        if bw == 1.4:
            sbw = "14"
        else:
            sbw = str(bw)
        
        for tm in ['tm1', 'tm4']:
            
            for direction in ['downlink', 'uplink']:
                
                if direction == 'downlink':    
                    schedulingl = ['dynamic']
                    sdirection = 'DIRECTION_DOWNLINK'
                elif direction == 'uplink':
                    schedulingl = ['min_mcs', 'max_mcs']
                    sdirection = 'DIRECTION_UPLINK'
                
                for scheduling in schedulingl:
                    
                    print("    def test_" + direction + "_" + tm + "_band" + str(b) + "_" + sbw + "MHz_" + scheduling + "(self):")
                    if scheduling == 'dynamic':
                        sscheduling = "SCHEDULING_DYNAMIC"
                    elif scheduling == 'min_mcs':
                        sscheduling = "SCHEDULING_MIN_MCS"
                    elif scheduling == 'max_mcs':
                        sscheduling = "SCHEDULING_MAX_MCS"
                        
                    print("\n        self.do_test(direction = " + sdirection + ", band = " + str(b) + ", scheduling = " + sscheduling + ", bandwidth = " + str(bw) + ", transmission_mode = " + str(tm).upper() + ", ca_band2 = 0)\n")
