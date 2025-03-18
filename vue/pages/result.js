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
      avgCount: 0,
      log: "",
      mt:"New",
      mOptions: ['New'],
    };
  },

  mixins: [mixinNotify],
  computed: {
    ...Vuex.mapState(["lastResult", "history", "results", "authenticate"]),
    ...Vuex.mapGetters(["getHistoryIndex", "getCurrentDate"]),
  },
  methods: {
    ...Vuex.mapActions(["history","loadResults", "sendCmd"]),
    update(name) {
      this.id = this.getHistoryIndex(name);
      if (this.id > -1) {
        this.loadResults([this.id]);
        this.isNew = false;
      }
    },
    onSubmit() {
      if (this.isNew && this.mt === 'New' && this.name.length > 1) 
      {
        this.sendCmd({
          new: {
            name: this.name,
            date: this.date,
            desc: this.description,
          },
        });
      } else if(this.isNew)
      {
        const id = this.getHistoryIndex(this.mt);
        if (id > -1)
        {
          this.sendCmd({
            add_avg: id
          });
        }
      }
      else if (this.id > -1) 
      {
        this.sendCmd({
          save: {
            id: this.id,
            desc: this.description,
          },
        });
      }
    },
    removeHtmlTags(html) {
      return html.replace(/<[^>]*>/g, ""); // Elimina todas las etiquetas HTML
    },
    onDownload() {
      sucess = JsonToCsv.saveWithDialog(
        this.chardata,
        this.name,
        this.date,
        this.removeHtmlTags(this.description)
      );
      if(sucess) 
        this.notify("CSV downloaded successfully.");
      else 
        this.notifyW("Error converting JSON to CSV");
    },
    onCopy() {
      sucess = JsonToCsv.copyToClipboard(
        this.chardata,
        this.name,
        this.date,
        this.removeHtmlTags(this.description)
      );
      if(sucess) 
        this.notify("CSV successfully copied to clipboard.");
      else 
        this.notifyW("Error copying to clipboard");
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

      this.mOptions = ["New" ];
      this.history.forEach((item) => {
        this.mOptions.push(item.name);
      });

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
      this.mOptions = ["New" ];
      newValue.forEach((item) => {
        this.mOptions.push(item.name);
      });
    },
    // abren un resultado guardado
    results: function (newValue, oldValue) {
      var doc = newValue[0];
      this.description = doc.description;
      this.date = doc.date;
      this.chardata = doc.data;
      this.name = doc.name;// TODO
      this.avgCount = doc.avg_count+1;
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
  
              <q-input v-if="authenticate && isNew && mt === 'New'" filled 
                v-model="name" label="name" dense 
                counter
                rules :rules="[
                  val => !!val || '* Required',
                  val => val.length > 1  || 'Please use minimum 2 character',
                  val => val.length < 38 || 'Please use maximum 39 characters',
                  validFileName
                ]"
              />
  
              <q-select filled v-if="authenticate && isNew" 
                v-model="mt" :options="mOptions" label="Update Average" />

              <div class="q-mt-xl"><b>Date :</b> {{date}}</div> 
              <div class="q-mt-md" v-if="!isNew"><b>n° tests :</b> {{avgCount}}</div> 
    
              <q-editor  v-if="authenticate" min-height="5rem" 
                v-model="description" />
              <div v-else v-html="description" />

              <div class="text-caption">{{ descCount }} / 200</div>
              <div v-if="descLimit" class="text-negative">
                The 200 character limit has been exceeded.
              </div>
  
              <div> 
                <q-btn label="Save" type="submit" v-if="authenticate" icon="icon-cloud_upload"
                  color="primary" style="min-width: 100px"/>
                <q-btn label="download" @click="onDownload()" v-if="authenticate" icon="icon-cloud_download"
                  color="primary" style="min-width: 100px"/>
                <q-btn label="copy" @click="onCopy()" v-if="authenticate" icon="icon-content_paste"
                  color="primary" style="min-width: 100px"/>
              </div>
    
            </q-form>
    
          </q-card-section>
          <q-badge multi-line>
            size: "{{ this.chardata.length }}"
            mt: "{{ mt }}"
          </q-badge>
          </q-card>
      </q-page>
      `,
};
