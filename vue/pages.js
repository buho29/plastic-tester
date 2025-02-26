//Paginas
const pageHome = {
  computed: {
    ...Vuex.mapState(["sensors"]),
  },
  template: /*html*/ `
  <q-page>
    <b-container title="Home">
      <div class="flex justify-center q-ma-none q-pa-none bg-grey-1">
        <b-sensor prop="Position" :value="sensors.d+'mm'"/>
        <b-sensor prop="Force" :value="sensors.f+'kg'"/>
      </div>
      <q-separator />
      <div class="q-pa-lg">
      
      <q-btn push label="Run Test" size="xl" class="q-ma-md bthome" color="primary"
          glossy stack icon="icon-directions_run" to="test"/>
      
      <q-btn push label="Move" size="xl" class="q-ma-md bthome" color="positive"
          glossy stack icon="icon-control_camera" to="move"/>
      
      
      <q-btn push label="Historial" size="xl"  class="q-ma-md bthome" color="accent"
          glossy stack icon="icon-trending_up" to="history"/>
      
      
      <q-btn push label="Opciones" size="xl" class="q-ma-md bthome" color="info"
          glossy stack icon="icon-settings" to="options"/>

      
      <q-btn push label="System" size="xl" class="q-ma-md bthome" color="warning"
          glossy stack icon="icon-build" to="system"/>
      
      </div >
    </b-container>
  </q-page>
  `,
};

const pageLogin = {
  data: function () {
    return {
      name: null,
      pass: null,
      isPwd: true,
    };
  },
  methods: {
    ...Vuex.mapActions(["setAuthentication"]),
    onSubmit() {
      this.setAuthentication({ name: this.name, pass: this.pass });
    },
    onReset() {
      this.name = null;
      this.pass = null;
    },
  },
  template: /*html*/ `
  <q-page>
    <b-container title="Login">
      <div class="q-pa-lg">
  
      <q-form @submit="onSubmit" @reset="onReset" class="q-gutter-sm" >
        
                <q-input filled v-model="name" label="Name" dense
                  lazy-rules :rules="[ val => val && val.length > 0 || 'Please type something']">
                </q-input>
                    
                <q-input v-model="pass" filled dense label="Password"
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
                    
                <div>
                    <q-btn label="Enviar" type="submit" color="primary"/>
                    
                    <q-btn label="Reset" type="reset" color="primary" flat/>
                </div>
              </q-form>
      
      </div >
    </b-container>
  </q-page>
  `,
};

const pageTest = {
  data: function () {
    return {
      tab: "test",
      config: {
        dist: 5,
        trigger: 0.3,
      },
    };
  },
  computed: {
    ...Vuex.mapState(["sensors"]),
  },
  methods: {
    ...Vuex.mapActions(["sendCmd"]),
    onStop() {
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
                  <q-btn glossy stack label="Go" icon="icon-home" 
                    @click="onHome()"/>
                  <q-btn glossy stack label="Tare" icon="icon-refresh"
                    @click="onTare()"/>
                </q-btn-group>
                <q-btn push label="STOP" size="xl" color="red" glossy stack icon="icon-error"  
                  @click="onStop"/>
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

const pageMove = {
  data: function () {
    return {
      dist_id: "3",
      dists: [0.5, 2, 10, 50],
    };
  },
  computed: {
    ...Vuex.mapState(["sensors"]),
  },
  methods: {
    ...Vuex.mapActions(["sendCmd"]),
    onStop() {
      this.sendCmd({ stop: 1 });
    },
    onMove(dir) {
      this.sendCmd({
        move: {
          dir: dir,
          dist: this.dists[this.dist_id],
        },
      });
    },
    // if go = 0 set home
    // if go = 1 go home
    onHome(go) {
      this.sendCmd({ sethome: go });
    },
  },
  template: /*html*/ `
  <q-page>
    <b-container title="Manual">
      <div class="flex justify-center bg-grey-1">
        <b-sensor prop="Position" :value="sensors.d+'mm'"/>
        <b-sensor prop="Force" :value="sensors.f+'kg'"/>
      </div>
      <q-separator />
      <q-card-section>
        <q-btn-toggle v-model="dist_id" push 
          toggle-color="primary" class="q-ma-md"
          :options="[
            {label: '0.5mm', value: '0'},
            {label: '2mm', value: '1'},
            {label: '10mm', value: '2'},
            {label: '50mm', value: '3'},
          ]"
        />
        <div class="q-pa-md">
          <q-btn-group class=" ">
            <q-btn glossy icon="icon-fast_rewind" 
              @click="onMove(-1)"/>
            <q-btn glossy icon="icon-fast_forward"  
              @click="onMove(1)"/>
            <q-btn glossy stack label="Set" icon="icon-home" @click="onHome(0)"/>
            <q-btn glossy stack label="Go" icon="icon-home" @click="onHome(1)"/>
          </q-btn-group>
          <q-btn push label="STOP" size="xl" color="red" class="q-mt-md"
            @click="onStop" glossy stack icon="icon-error "/>
        </div>
      </q-card-section>
    </b-container>
  </q-page>
  `,
};

const pageHistory = {
  data: function () {
    return {
      mResults: [],
      chardata: [],
      historyLen:0,
      mProp: "f",
      log: "",
      options: [
        { label: "Force", value: "f" },
        { label: "Distance", value: "d" },
        { label: "Time", value: "t" },
      ],
    };
  },
  computed: {
    ...Vuex.mapState(["history", "results", "authenticate"]),
    ...Vuex.mapGetters(["getHistoryIndex"]),
  },
  methods: {
    ...Vuex.mapActions(["loadResults", "sendCmd"]),
    update(evt) {
      this.historyLen = this.history.length;
      let indexes = [];
      this.mResults.forEach((item) => {
        let index = this.getHistoryIndex(item.name);
        indexes.push(index);
      });
      this.log = indexes;
      if (indexes.length > 0) this.loadResults(indexes);
    },
    deleteItem(name) {
      this.log = name;
      let index = this.getHistoryIndex(name);
      if (index > -1)
        this.sendCmd({
          delete: index,
        });
    },
    editItem(name) {
      this.log = name;
      this.$router.push({ path: `/result/${name}` });
    },
    getLength() {
      return this.history.length;
    },
  },
  mounted() {
    console.log(this.history.length,"gggggggggggggg")
  },
  watch: {
    // si abren directamente la pagina
    history: function (newValue, oldValue) {
      this.update();
      console.log(this.history.length,oldValue.length)
    },
    //
    results: function (newValue, oldValue) {
      this.chardata = newValue;
      console.log("update results:");
    },
  },
  template: /*html*/ `
  <q-page>
    
    <b-container title="Compare Results">
      <div class="row">
        <q-select filled 
          style="minWidth: 170px" class="col q-ma-sm"
          label="Select results" multiple
          v-model="mResults" :options="history"
          emit-value map-options
          option-label="name" @popup-hide="update">
  
          <template v-slot:option="{ itemProps, itemEvents, opt, selected, toggleOption }">
            <q-item v-bind="itemProps" 
              v-on="itemEvents">
  
              <q-item-section>
                <q-item-label v-html="opt.name" />
                <q-item-label v-html="opt.date" />
              </q-item-section>
  
              <q-item-section side>
                <q-toggle :value="selected" @input="toggleOption(opt)" />
              </q-item-section>
  
            </q-item>
          </template>
        </q-select>
  
        <q-select filled label="Filter" class="col q-ma-sm"
          v-model="mProp" :options="options"
          emit-value map-options
        />
      </div>
        
      <compare-chart v-if="mResults.length > 0" 
        :rawData="chardata" :prop="mProp"/>
        
    </b-container>
    <b-container :title="'Manage Results ('+ history.length + '/10)'">
        <q-list bordered >
          <transition-group name="list-complete">
            <q-item v-for="result in history" :key="result.name"
              v-ripple class="list-complete-item"
            >
              <q-item-section>
                <q-item-label>{{ result.name }}</q-item-label>
              </q-item-section>
              <q-item-section side>
                <div class=" q-gutter-sm text-white" style="display:flex;">
                  <q-btn
                    icon="icon-edit" class="bg-primary"
                    @click="editItem(result.name)"
                  />
                  <q-btn v-if="authenticate"
                    icon="icon-delete_forever" class="bg-warning"
                    @click="deleteItem(result.name)"
                  />
                </div>
              </q-item-section>
            </q-item>
          </transition-group>
        </q-list>
    </b-container>
  </q-page>
        `,
};

const pageOptions = {
  data: function () {
    return {
      isPwd: true,
      tab: "general",
      options: {
        // wifi
        wifi_ssid: null,
        wifi_pass: null,
        // user
        www_pass: null,
        www_user: null,
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

const pageSystem = {
  data: function () {
    return {
      tab: "system",
    };
  },
  computed: {
    ...Vuex.mapState(["authenticate", "isConnected", "system"]),
  },
  methods: {
    ...Vuex.mapActions(["loadDataAuth", "restart"]),
    loadSystem() {
      if (this.authenticate && this.isConnected)
      //nos registramos a eventos auth al servidor
      //q nos devolvera los datos
      this.loadDataAuth(1);
    },
  },
  mounted() {
    this.loadSystem();
  },
  beforeDestroy() {
    //this.loadDataAuth(-1);
  },
  watch: {
    //si abren directamente la pagina
    isConnected: function (newValue, oldValue) {
      //nos registramos a eventos auth al servidor
      //para cargar datos
      if (this.authenticate && newValue) this.loadDataAuth(1);
    },
  },
  template: /*html*/ `
   <q-page>
  
      <div class="q-my-lg q-mx-auto text-center page bg-white">
        
        <div>
          
          <q-tabs v-model="tab"
              class="bg-primary text-white shadow-1"
              indicator-color="accent" align="justify"
          >
            <q-tab name="system" label="system"/>
            <q-tab name="icons" label="icons fonts" />
          </q-tabs>

          <q-tab-panels v-model="tab" animated>

            <q-tab-panel name="system">
              
                <!-- Iterar sobre las categorías principales (WIFI, HEAP, FLASH, etc.) -->
                <div class="q-px-xl q-py-sm"
                  v-for="(category, categoryName) in system" :key="categoryName">
                  
                  <p class="q-pa-none q-ma-none">{{ categoryName }}</p>

                  <!-- Iterar sobre las propiedades de cada categoría -->
                  <div v-for="(value, key) in category" :key="key" class="row q-mx-auto">
                    <div class="col text-bold text-left text-capitalize">{{ key }}</div>
                    <div class="col text-right"> {{ value }} </div>
                  </div>
                </div>
                <q-btn color="primary" class="q-ma-sm"
                  label="Reset Esp32" @click="restart" />
                <q-btn color="primary" class="q-ma-sm"
                  label="Update Info" @click="loadSystem" />
            </q-tab-panel>

            <q-tab-panel name="icons">
              <div class="text-subtitle1"> {{Object.keys(icomoon).length}} Fonts </div>
              <div class="q-pa-md row items-start">
                <div v-for="(value, key) in icomoon" style="width: 70px;">
                  <q-icon :name="key" style="font-size: 2em;"/>
                  <p>{{key}}</p>    
                </div>
              </div>            
            </q-tab-panel>

          </q-tab-panels>
        </div>
      </div>
    </q-page>
    `,
};

const pageResult = {
  data: function () {
    return {
      name: null,
      date: "",
      description: "",
      id: -1,
      chardata: [],
      isNew: false,
      descLimit: false,
      descCount: 0,
      log: "",
    };
  },
  computed: {
    ...Vuex.mapState(["lastResult", "history", "results", "authenticate"]),
    ...Vuex.mapGetters(["getHistoryIndex", "getCurrentDate"]),
  },
  methods: {
    ...Vuex.mapActions(["loadResults", "sendCmd"]),
    update(name) {
      this.id = this.getHistoryIndex(name);
      if (this.id > -1) {
        this.loadResults([this.id]);
        this.isNew = false;
      }
    },
    onSubmit() {
      if (this.isNew) {
        this.sendCmd({
          new: {
            name: this.name,
            date: this.date,
            desc: this.description,
          },
        });
      } else if (this.id > -1) {
        this.sendCmd({
          save: {
            id: this.id,
            desc: this.description,
          },
        });
      }
    },
    validFileName(value) {
      // Expresión regular para validar el nombre del fichero
      const regex = /^[a-zA-Z0-9_\-\s]+$/;
      return (
        regex.test(value) ||
        "Invalid filename. Please use only letters, numbers, spaces and -_"
      );
    },
  },
  mounted() {
    this.name = this.$route.params.name;

    if (this.name === "n") {
      // si el resultado esta vacio (historial back)
      // volvemos atras
      if (this.lastResult.length === 0) {
        this.$router.go(-1);
        return;
      }

      this.isNew = true;
      this.name = "";

      this.date = this.getCurrentDate();
      this.chardata = this.lastResult;
      console.log("update last_result:");
    } else {
      this.update(this.$route.params.name);
    }
  },
  //alguien escribe una url en el navegador
  beforeRouteUpdate(to, from, next) {
    this.update(to.params.name);
    console.log("update route");
    next();
  },
  watch: {
    // si abren directamente la pagina
    history: function (newValue, oldValue) {
      this.update(this.$route.params.name);
    },
    // abren un resultado guardado
    results: function (newValue, oldValue) {
      var doc = newValue[0];
      this.description = doc.description;
      this.date = doc.date;
      this.chardata = doc.data;
    },
    //
    description: function (newValue, oldValue) {
      this.descCount = newValue.length;
      if (this.descCount > 200) {
        this.descLimit = true;
      } else this.descLimit = false;
    },
  },
  template: /*html*/ `
    <q-page class="q-pa-none text-center">
      <q-card class="q-ma-md">
        <q-card-section class="q-pa-none q-ma-none">
          <div class="bg-primary shadow-3">
            <div v-if="isNew" class="text-h6 text-white q-pa-sm text-center">New</div>
            <div v-else class="text-h6 text-white q-pa-sm text-center">{{name}}</div>
          </div>
        </q-card-section>
        <q-card-section>

          <result-chart :raw-data="chardata"/>
  
          <q-form @submit="onSubmit" class="q-gutter-md ">

            <q-input v-if="authenticate && isNew" filled 
              v-model="name" label="name" dense 
              counter
              rules :rules="[
                val => !!val || '* Required',
                val => val.length > 1  || 'Please use minimum 2 character',
                val => val.length < 38 || 'Please use maximum 39 characters',
                validFileName
              ]"
            />

            <div class="q-mt-xl"><b>Date :</b> {{date}}</div> 
  
            <q-editor  v-if="authenticate" min-height="5rem" 
              v-model="description" />
            <div v-else v-html="description" />
            <div class="text-caption">{{ descCount }} / 200</div>
            <div v-if="descLimit" class="text-negative">
              The 200 character limit has been exceeded.
            </div>

            <div  > 
              <q-btn label="Save" type="submit" v-if="authenticate"
                color="primary" style="min-width: 100px"/>
            </div>
  
          </q-form>
  
        </q-card-section>
        <q-badge multi-line>
          size: "{{ this.chardata.length }}"
        </q-badge>
        </q-card>
    </q-page>
    `,
};
