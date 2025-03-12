
const pageMove = {
    data: function () {
      return {
        dist_id: "0",
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
          <div class="q-pb-md">
            <q-btn-group>
              <q-btn glossy stack label="left"icon="icon-fast_rewind" 
                @click="onMove(-1)"/>
              <q-btn glossy stack label="right"icon="icon-fast_forward"  
                @click="onMove(1)"/>
              <q-btn glossy stack label="pause" icon="icon-pause"  
                @click="onMove(0)"/>
            </q-btn-group>
          </div>
            <div class="q-pb-md"><q-btn-group>
              <q-btn glossy stack label="Set 0" icon="icon-home" @click="onHome(0)"/>
              <q-btn glossy stack label="Go" icon="icon-home" @click="onHome(1)"/>
            </q-btn-group></div>
            <div class="q-pb-md">
              <q-btn push label="STOP" size="xl" color="red" class="q-mt-md"
                @click="onStop" glossy stack icon="icon-error "/>
            </div>

        </q-card-section>
      </b-container>
    </q-page>
    `,
  };