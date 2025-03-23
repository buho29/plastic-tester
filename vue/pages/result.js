const pageResult = {
  data: function () {
    return {
      name: null,
      date: "",
      description: "",
      avgCount: 0,
      length: 5,
      area: 2,
      id: -1,
      chardata: [],
      isNew: false,
      descLimit: false,
      descCount: 0,
      log: "",
      mt: "New",
      mOptions: ["New"],
      showPopup: false,
      elongation:0,
      strain:0,
      force:0,
    };
  },

  mixins: [mixinNotify],
  computed: {
    ...Vuex.mapState(["lastResult", "history", "results", "authenticate"]),
    ...Vuex.mapGetters(["getHistoryIndex", "getCurrentDate"]),
  },
  methods: {
    ...Vuex.mapActions(["history", "loadResults", "sendCmd"]),
    update(name) {
      this.id = this.getHistoryIndex(name);
      if (this.id > -1) {
        this.loadResults([this.id]);
        this.isNew = false;
      }
    },
    onSubmit() {
      if (this.isNew && this.mt === "New" && this.name.length > 1) {
        this.sendCmd({
          new: {
            name: this.name,
            date: this.date,
            desc: this.description,
            length: this.length,
            area: this.area
          },
        });
      } else if (this.isNew) {
        const id = this.getHistoryIndex(this.mt);
        if (id > -1) {
          this.sendCmd({
            add_avg: id,
          });
        }
      } else if (this.id > -1) {
        this.sendCmd({
          save: {
            id: this.id,
            desc: this.description,
          },
        });
      }
    },
    onRename() {
      if (this.name.length > 1 && this.id > -1) {
        this.sendCmd({
          rename: {
            id: this.id,
            name: this.name,
          },
        });
        console.log("rename");  
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
        this.removeHtmlTags(this.description),
        this.avgCount
      );
      if (sucess) this.notify("CSV downloaded successfully.");
      else this.notifyW("Error converting JSON to CSV");
    },
    onCopy() {
      sucess = JsonToCsv.copyToClipboard(
        this.chardata,
        this.name,
        this.date,
        this.removeHtmlTags(this.description),
        this.avgCount
      );
      if (sucess) this.notify("CSV successfully copied to clipboard.");
      else this.notifyW("Error copying to clipboard");
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

      this.mOptions = ["New"];
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
      this.mOptions = ["New"];
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
      this.name = doc.name; // TODO
      this.avgCount = doc.avg_count + 1;
      this.length = doc.length;
      this.area = doc.area;
      const item_break = this.chardata.find(item => item.t === 0);
      if (item_break) {
        this.elongation = Math.round(item_break.d / this.length* 10000) / 100;
        this.strain = Math.round(item_break.f * 9.91/ this.area * 100) / 100;
        this.force = Math.round(item_break.f * 9.91 * 100) / 100;
      }
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
        <q-card class="q-ma-md q-my-lg q-mx-auto page">
          <q-card-section class="q-pa-none q-ma-none">
            <div class="row bg-primary shadow-3 q-pa-sm text-h6 text-white ">
              <div v-if="isNew" class="col-grow self-center  text-center">New</div>
              <div v-else class="col-grow self-center text-center">{{name}}</div>
              <q-btn v-if="authenticate && !isNew"  @click="showPopup=true"
                flat round color="white" icon="icon-edit"/>
            </div>
          </q-card-section>
          <q-card-section>
  
            <result-chart :raw-data="chardata"/>

            <q-form @submit="onSubmit" >
  
              <q-input v-if="authenticate && isNew && mt === 'New'" filled 
                 class="q-py-md"
                v-model="name" label="name" dense 
                counter
                rules :rules="[
                  val => !!val || '* Required',
                  val => val.length > 1  || 'Please use minimum 2 character',
                  val => val.length < 38 || 'Please use maximum 39 characters',
                  validFileName
                ]"
              />

              <div v-if="isNew && mt === 'New'" class="text-center q-py-md q-mx-auto">
                <div class="row q-gutter-md">
                  <q-input class="col" dense
                    step="any" filled type="number" v-model.number="length" label="length" hint="The initial length of the breakable Test specimen(in mm)"
                    lazy-rules :rules="[
                      val => val !== null && val !== '' || 'Please type something',
                      val => val > 0 && val < 50 || 'wrong value'
                    ]"
                  />
                  <q-input class="col" dense
                    step="any" filled type="number" v-model.number="area" label="area" hint="The cross-section of the Test specimen(in mm²)"
                    lazy-rules :rules="[
                      val => val !== null && val !== '' || 'Please type something',
                      val => val > 0 && val < 100 || 'wrong value'
                    ]"
                  />
                </div>
              </div>

              <q-select filled v-if="authenticate && isNew" 
                class="q-py-md"
                v-model="mt" :options="mOptions" label="Update Average" />

              <div v-if="!isNew" class="text-center q-py-md q-mx-auto" 
                style="max-width:300px;">
                <b>Test result</b>
                <div class="row">
                  <div class="col text-bold text-left text-capitalize">Date:</div>
                  <div class="col text-right">{{date}}</div>
                </div>
                <div class="row" >
                  <div class="col text-bold text-left text-capitalize">n° tests:</div>
                  <div class="col text-right">{{avgCount}}</div>
                </div>
                <div class="row">
                  <div class="col text-bold text-left text-capitalize">avg break  force:</div>
                  <div class="col text-right"> {{force}} N</div>
                </div>
                <div class="row">
                  <div class="col text-bold text-left text-capitalize">Test specimen:</div>
                  <div class="col text-right"> {{length}}mm * {{area}}mm²</div>
                </div>
                <div class="row">
                  <div class="col text-bold text-left text-capitalize">avg break strain:</div>
                  <div class="col text-right"> {{strain}} N/mm²</div>
                </div>
                <div class="row">
                  <div class="col text-bold text-left text-capitalize">avg break  elongation:</div>
                  <div class="col text-right"> {{elongation}} %</div>
                </div>
              </div>

              
              <q-editor  v-if="authenticate" min-height="5rem" 
                v-model="description" />
              <div v-else v-html="description" />

              <div class="text-caption">{{ descCount }} / 200</div>
              <div v-if="descLimit" class="text-negative">
                The 200 character limit has been exceeded.
              </div>
  
              <div  class="q-gutter-sm"> 
                <q-btn stack label="Save" type="submit" v-if="authenticate&& mt === 'New'" icon="icon-cloud_upload"
                  color="primary" />
                <q-btn stack label="Update" type="submit" v-if="authenticate&& mt !== 'New'" icon="icon-cloud_upload"
                  color="primary" />
                <q-btn stack label="download" @click="onDownload()" v-if="authenticate" icon="icon-cloud_download"
                  color="primary" />
                <q-btn stack label="copy" @click="onCopy()" v-if="authenticate" icon="icon-content_paste"
                  color="primary" />
                <q-btn stack label="Test" to="/test" v-if="authenticate" icon="icon-directions_run"
                  color="primary" />
              </div>
    
            </q-form>
    
          </q-card-section>
          </q-card>
        <q-dialog v-model="showPopup" persistent>
          <q-card style="min-width: 350px">
            <q-card-section>
              <div class="text-h6">New name</div>
            </q-card-section>

            <q-card-section class="q-pt-none">
  
              <q-input  v-model="name" label="name" counter
                rules :rules="[
                  val => !!val || '* Required',
                  val => val.length > 1  || 'Please use minimum 2 character',
                  val => val.length < 38 || 'Please use maximum 39 characters',
                  validFileName
                ]"
              />
            </q-card-section>

            <q-card-actions align="right" class="text-primary">
              <q-btn flat label="Cancel" v-close-popup />
              <q-btn flat label="Rename" @click="onRename" v-close-popup />
            </q-card-actions>
          </q-card>
        </q-dialog>
      </q-page>
      `,
};
