
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
        const indexes = [];
        this.mResults.forEach((item) => {
          const index = this.getHistoryIndex(item.name);
          indexes.push(index);
        });
        this.log = indexes;
        if (indexes.length > 0) this.loadResults(indexes);
      },
      deleteItem(name) {
        this.log = name;
        const index = this.getHistoryIndex(name);
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
    watch: {
      // si abren directamente la pagina
      history: function (newValue, oldValue) {
        this.update();
      },
      //
      results: function (newValue, oldValue) {
        this.chardata = newValue;
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
  `};