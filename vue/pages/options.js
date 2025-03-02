
const pageOptions = {
    data: function () {
      return {
        isPwd: true,
        tab: "general",
        options: {
          // wifi
          wifi_ssid: null, wifi_pass: null,
          // user
          www_pass: null, www_user: null,
          // motor
          speed: null,
          acc_desc: null,
          screw_pitch: null,
          micro_step: null,
          invert_motor: null,
          // general
          home_pos: null,
          max_travel: null,
          max_force: null,
        },
        log: "", // log
      };
    },
    computed: {
      ...Vuex.mapState(["authenticate", "isConnected", "config"]),
    },
    methods: {
      ...Vuex.mapActions(["loadDataAuth", "editConfig"]),
      onSubmit() {
        switch (this.tab) {
          case "general":
            this.editConfig({
              home_pos: this.options.home_pos,
              max_travel: this.options.max_travel,
              max_force: this.options.max_force,
            });
            break;
          case "motor":
            this.editConfig({
              screw_pitch: this.options.screw_pitch,
              micro_step: this.options.micro_step,
              speed: this.options.speed,
              acc_desc: this.options.acc_desc,
              invert_motor: this.options.invert_motor,
            });
            break;
        }
      },
      async onSubmitAccount() {
        // Validar el segundo formulario
        const isUserValid = await this.$refs.user.validate();
        if (isUserValid) {
          this.editConfig({
            www_user: this.options.www_user,
            www_pass: this.options.www_pass,
          });
        }
  
        // Validar el primer formulario
        const isWifiValid = await this.$refs.wifi.validate();
        if (isWifiValid) {
          this.editConfig({
            wifi_ssid: this.options.wifi_ssid,
            wifi_pass: this.options.wifi_pass,
          });
        }
      },
    },
    mounted() {
      if (this.authenticate && this.isConnected)
        //nos registramos a eventos auth al servidor
        //q nos devolvera los datos
        this.loadDataAuth(0);
    },
    beforeDestroy() {
      //this.loadDataAuth(-1);
    },
    watch: {
      //si abren directamente la pagina
      isConnected: function (newValue, oldValue) {
        //para cargar datos
        if (this.authenticate && newValue) this.loadDataAuth(0);
      },
      //si actualiza config
      config: function (newValue, oldValue) {
        //actualizamos los datos cargados
        for (const key in newValue) {
          this.options[key] = newValue[key];
        }
      },
    },
    template: /*html*/ `
      <q-page>
    
        <div class="q-ma-lg q-mx-auto items-center page bg-white">
          
          <div class="" >
            
            <q-tabs
              v-model="tab"
                class="bg-primary text-white shadow-1"
                indicator-color="accent" align="justify"
            >
              <q-tab name="general" label="General"/>
              <q-tab name="motor" label="Motor" />
              <q-tab name="accounts" label="Accounts" />
            </q-tabs>
    
    
            <q-tab-panels v-model="tab" animated>
    
              <q-tab-panel name="general">
                  <q-form @submit="onSubmit" class="q-gutter-md " >
                                                        
                    <q-input step="any" filled type="number" v-model.number="options.home_pos" label="Home position" hint="The initial position in which the test will begin"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 10 && val < 500 || 'wrong value'
                      ]"
                    />
    
                    <q-input step="any" filled type="number" v-model.number="options.max_travel" label="Max travel" hint="The maximum distance that can be traveled (in mm)"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 20 && val < 600 || 'wrong value'
                      ]"
                    />
                    
                    <q-input step="any" filled type="number" v-model.number="options.max_force" label="Max force" hint="The maximum force that the sensor/motor can withstand (in Kg)"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 1 && val < 100 || 'wrong value'
                      ]"
                    /> 
  
                    <div class="text-center">
                      <q-btn label="Save" type="submit" color="primary"/>
                    </div>
    
                 </q-form>
              </q-tab-panel>
    
              <q-tab-panel name="motor">
                <q-form @submit="onSubmit" class="q-gutter-md " >
                                                                                                                                                                              
                    <q-input step="any" filled type="number" v-model.number="options.speed" label="Speed" hint="mm/sc"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 1 && val < 100 || 'wrong value'
                      ]"
                    />
    
                    <q-input step="any" filled type="number" v-model.number="options.acc_desc" label="Acc/Desc" hint="mm/sc²"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 1 && val < 100 || 'wrong value'
                      ]"
                    />
                                                              
                    <q-input step="any" filled type="number" 
                      v-model.number="options.screw_pitch" label="Screw pitch" hint="mm"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 1 && val < 100 || 'wrong value'
                      ]"
                    />
    
                    <q-input filled type="number" v-model.number="options.micro_step" label="Driver microsteep" hint="1/value"
                      lazy-rules :rules="[
                        val => val !== null && val !== '' || 'Please type something',
                        val => val > 1 && val < 100 || 'wrong value'
                      ]"
                    />
  
                    <q-item tag="label" v-ripple>
                      <q-item-section >
                        <q-item-label>Invert motor</q-item-label>
                        <q-item-label caption>If the motor does not go counter-clockwise to go right, allow this.</q-item-label>
                      </q-item-section>
                      <q-item-section avatar>
                        <q-toggle v-model="options.invert_motor"/>
                      </q-item-section>
                    </q-item>
  
                    <div class="text-center">
                      <q-btn label="Save" type="submit" color="primary"/>
                    </div>
    
                 </q-form>
              </q-tab-panel>
    
              <q-tab-panel name="accounts">
  
                <q-form  ref="wifi" class="q-gutter-sm" >
                  <q-item-label header>Wifi</q-item-label>
          
                  <q-input filled v-model="options.wifi_ssid" label="SSID(wifi)" dense
                    lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']">
                  </q-input>
                  <q-input v-model="options.wifi_pass" filled dense label="Password"
                    lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']"
                    :type="isPwd ? 'password' : 'text'">
                    
                    <template v-slot:append>
                      <q-icon
                        :name="isPwd ? 'icon-visibility_off' : 'icon-remove_red_eye'"
                        class="cursor-pointer"
                        @click="isPwd = !isPwd"
                      />
                    </template>
                  </q-input>
                      
              </q-form>
              <q-separator spaced />      
              <q-form  ref="user" class="q-gutter-sm" >
              
                <q-item-label header>User</q-item-label>
  
                <q-input filled v-model="options.www_user" label="Name" dense
                  lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']">
                </q-input>
                <q-input v-model="options.www_pass" filled dense label="Password"
                    lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']"
                    :type="isPwd ? 'password' : 'text'">
                    
                    <template v-slot:append>
                      <q-icon
                        :name="isPwd ? 'icon-visibility_off' : 'icon-remove_red_eye'"
                        class="cursor-pointer"
                        @click="isPwd = !isPwd"
                      />
                    </template>
                  </q-input>        
              </q-form>
  
              <div class="text-center">
                  <q-btn label="Save" @click="onSubmitAccount" color="primary"/>
              </div>
              </q-tab-panel>           
            </q-tab-panels>
    
          </div>
    
        </div>
      </q-page>
      `,
  };