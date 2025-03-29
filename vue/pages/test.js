const pageTest = {
  data: function () {
    return {
      tab: "test",
      config: {
        dist: 10,
        trigger: 0.3,
        speed: 1,
        acc_desc: 2
      },
    };
  },
  computed: {
    ...Vuex.mapState(["sensors"]),
  },
  methods: {
    ...Vuex.mapActions(["sendCmd"]),
    onStopPanic() {
      this.sendCmd({ stop: 1 });
    },
    onRun() {
      this.sendCmd({ run: this.config });
    },
    onHome() {
      this.sendCmd({ sethome: 1 });
    },
    onTare() {
      this.sendCmd({ tare: 1 });
    },      
    onStop() {
      this.sendCmd({
        move: {
          dir: 0,
          dist: 0,
        },
      });
    },
  },
  template: /*html*/ `
    <q-page>
        <div class="q-ma-lg q-mx-auto text-center items-center page bg-white">
          
          <div>
            
            <q-tabs v-model="tab"
                class="bg-primary text-white shadow-1"
                indicator-color="accent" align="justify"
            >
              <q-tab name="test" label="New Test"/>
              <q-tab name="config" label="Config Test" />
            </q-tabs>
    
    
            <q-tab-panels v-model="tab" animated>
              <q-tab-panel name="test" class="q-ma-none q-pa-none">
                <div class="flex justify-center bg-grey-1">
                  <b-sensor prop="Position" :value="sensors.d+'mm'"/>
                  <b-sensor prop="Force" :value="sensors.f+'kg'"/>
                </div>
                <q-separator />
                <q-card-section class="q-pa-lg">
                  <q-btn-group class="q-ma-lg">
                    <q-btn glossy stack label="Run" icon="icon-directions_run" 
                      @click="onRun()" />
                  <q-btn glossy stack label="stop" icon="icon-stop"  
                    @click="onStop()"/>
                    <q-btn glossy stack label="Go" icon="icon-home" 
                      @click="onHome()"/>
                    <q-btn glossy stack label="Tare" icon="icon-refresh"
                      @click="onTare()"/>
                    <q-btn glossy stack label="Config" icon="icon-build"
                      @click="tab = 'config'"/>
                  </q-btn-group>
                  <q-btn push label="STOP" size="xl" color="red" glossy stack icon="icon-error"  
                    @click="onStopPanic"/>
                </q-card-section>
              </q-tab-panel>
              <q-tab-panel name="config">

                <q-input step="any" filled type="number" v-model.number="config.dist" label="Distance" 
                class="q-my-md"
                hint="Maximum distance to be traveled (in mm)"
                  lazy-rules :rules="[
                    val => val !== null && val !== '' || 'Please type something',
                    val => val > 1 && val < 10 || 'wrong value'
                  ]"
                />
    
                <q-input step="any" filled type="number" 
                  v-model.number="config.trigger" label="Trigger"  hint="Starts saving measurements from triggered (in Kg)"
                  lazy-rules :rules="[
                    val => val !== null && val !== '' || 'Please type something',
                    val => val > 0 && val < 5 || 'wrong value'
                  ]"
                />

                <q-input step="any" filled type="number" v-model.number="config.speed" label="Speed" hint="mm/sc" class="q-my-md"
                  lazy-rules :rules="[
                    val => val !== null && val !== '' || 'Please type something',
                    val => val >= 0.1 && val < 100 || 'wrong value'
                  ]"
                />
    
                <q-input step="any" filled type="number" v-model.number="config.acc_desc" label="Acc/Desc" hint="mm/sc²"
                  lazy-rules :rules="[
                    val => val !== null && val !== '' || 'Please type something',
                    val => val > 1 && val < 100 || 'wrong value'
                  ]"
                /> 
                <div class="q-ma-md">
                  <q-btn label="Back" 
                    @click="tab = 'test'"color="primary"/>
                </div>             
              </q-tab-panel>
            </q-tab-panels>
        
          </div>
        </div>
    </q-page>
    `,
};
