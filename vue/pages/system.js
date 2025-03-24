
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
              <q-tab name="files" label="files system" />
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

              
              <q-tab-panel name="files">
                <b-card title="Files">
                  <b-files/>
                </b-card>
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
  