// mixin para insertar notificaciones
const mixinNotify = {
  methods: {
    notify: function (msg) {
      //quasar notify
      this.$q.notify({
        color: "primary",
        textColor: "white",
        icon: "icon-cloud_done",
        message: msg,
      });
    },
    notifyW: function (msg) {
      this.$q.notify({
        color: "accent",
        textColor: "dark",
        icon: "icon-warning",
        message: msg,
      });
    },
  },
};

//vue components
Vue.component("b-sensor", {
  props: ["prop", "value"],
  template: /*html*/ `
    <q-card class="text-subtitle1 q-ma-lg">
      <q-card-section class="q-pa-none text-primary" style="min-width:100px;">
        <div class="q-pa-sm">{{ prop }}</div>
        <q-separator />
      </q-card-section>
      <q-card-section>
        <div class="text-dark">{{ value }}</div>
      </q-card-section>
    </q-card>
  `,
});

Vue.component("b-container", {
  props: ["title"],
  template: /*html*/ `
  <q-card
    class="q-my-lg q-mx-auto text-center bg-white page"
  >
    <q-card-section class="q-pa-none q-ma-none">
      <div class="bg-primary text-white shadow-3 round-top">
        <div class="q-pa-sm text-h6 ">{{ title }}</div>
      </div>
    </q-card-section>
    <q-card-section class="q-pa-none">
      <slot></slot>
    </q-card-section>
  </q-card>
  `,
});

Vue.component("compare-chart", {
  data() {
    return {
      chartData: null,
      options: {
        scales: {
          y: {
            beginAtZero: true,
            grid: { display: true },
          },
          x: {
            beginAtZero: true,
            grid: { display: false },
          },
        },
        legend: { display: false },
        responsive: true,
        maintainAspectRatio: false,
        height: 200,
      },
    };
  },
  props: ["rawData"],
  extends: VueChartJs.Bar,
  mixins: [VueChartJs.mixins.reactiveData],
  methods: {
    update() {
      //console.log(JSON.stringify(this.rawData));

      const labels = this.rawData.map((item) => item.name);

      const dValues = this.rawData.map((item) =>
        (
          (item.data.find((entry) => entry.t === 0).d / item.length) *
          100
        ).toFixed(3)
      );
      const fValues = this.rawData.map((item) =>
        (
          (item.data.find((entry) => entry.t === 0).f * 9.91) /
          item.area
        ).toFixed(2)
      );

      const datasets = [
        {
          label: "Elongation (%)",
          data: dValues,
          backgroundColor: "rgba(54, 162, 235, 0.6)",
          borderWidth: 1,
        },
        {
          label: "Strain (N/mm2)",
          data: fValues,
          backgroundColor: "rgba(255, 99, 132, 0.6)",
          borderWidth: 1,
        },
      ];

      // Actualizar chartData
      this.chartData = {
        labels: labels,
        datasets: datasets,
      };
    },
  },
  watch: {
    rawData: function (newQuestion, oldQuestion) {
      this.update();
    },
  },
  mounted() {
    this.update();
    this.renderChart(this.chartData, this.options);
  },
});

Vue.component("result-chart", {
  extends: VueChartJs.Line,
  mixins: [VueChartJs.mixins.reactiveData],
  props: ["rawData"],
  data() {
    return {
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          x: {
            type: "linear",
            position: "bottom",
            title: { display: true, text: "Distance (mm)" },
          },
          y: {
            title: { display: true, text: "Force (kg)" },
          },
        },
        plugins: {
          tooltip: {
            filler: {
              propagate: true,
            } /*
            callbacks: {
              label: (context) => {
                const dataPoint = this.rawData[context.dataIndex];
                return `Force: ${dataPoint.f}kg,/n Distance: ${dataPoint.d}mm`;
              }
            }*/,
          },
        },
      },

      chartData: {
        labels: [],
        datasets: [],
      },
    };
  },
  mounted() {
    this.updateChart();
    this.renderChart(this.chartData, this.options);
  },
  methods: {
    makeDataSet(
      label,
      data,
      borderColor,
      backgroundColor,
      pointStyle = true,
      fill = false
    ) {
      return {
        label: label,
        data: data,
        borderColor: borderColor,
        backgroundColor: backgroundColor,
        fill: fill,
        lineTension: 0.2,
        pointRadius: 3,
        pointHoverRadius: 8,
        pointStyle: pointStyle ? "circle" : false,
      };
    },
    updateChart() {
      // Crear labels y datos
      const arrTime = this.rawData.map((item) => item.d); // Tiempo (eje X)
      const arrForce = this.rawData.map((item) => ({ x: item.d, y: item.f }));
      const arrMin = this.rawData.map((item) => ({ x: item.d, y: item.mi }));
      const arrMax = this.rawData.map((item) => ({ x: item.d, y: item.ma }));

      const datasets = [
        this.makeDataSet(
          "Avg",
          arrForce,
          "rgb(54, 162, 235)",
          "rgb(54, 162, 235)"
        ),
        this.makeDataSet(
          "Max",
          arrMax,
          "rgba(255, 99, 132,0.6)",
          "rgba(201, 220, 248, 0.2)",
          false,
          "+1"
        ),
        this.makeDataSet(
          "Min",
          arrMin,
          "rgb(201, 220, 248)",
          "rgba(201, 220, 248, 0.2)",
          false,
          "-1"
        ),
      ];

      // Actualizar chartData
      this.chartData = {
        labels: arrTime,
        datasets: datasets,
      };
    },
  },
  watch: {
    rawData: function (newQuestion, oldQuestion) {
      this.updateChart();
      //console.log(JSON.stringify(this.rawData));
    },
  },
});

Vue.component("b-files", {
  data() {
    return {
      file: null, //file data
      root: [], //select
      folder: [], //select
      indexRoot: 0, //usado para reselecionar el select
      indexFolder: 0,
    };
  },
  computed: {
    //importamos datos
    ...Vuex.mapState(["rootFiles"]),
  },
  methods: {
    // importamos acciones
    ...Vuex.mapActions(["downloadItem", "deleteItem", "uploadItem"]),
    submitFile() {
      this.uploadItem({
        file: this.file,
        params: { path: this.folder.path },
      });
    },
    rootClick() {
      //selecionar la primera carpeta
      this.folder = this.root.folders[0];
      this.indexFolder = 0;
      //guardar index
      this.indexRoot = this.getIndex(this.root.path, this.rootFiles);
    },
    folderClick() {
      //guardar index
      this.indexFolder = this.getIndex(this.folder.path, this.root.folders);
    },
    getIndex(path, array) {
      return array.findIndex((file) => file.path === path);
    },
    // los datos cambiaron
    update() {
      //volver a seleccionar los selects con los index
      this.root = this.rootFiles[this.indexRoot];
      this.folder = this.root.folders[this.indexFolder];
    },
    // devuelve el path completo
    getPath(file) {
      const path = this.root.path;
      let folder = this.folder.path;
      if (path+"/" !== folder) folder += "/";
      return folder + file;
    },
    formatSize(size) {
      return Quasar.format.humanStorageSize(size);
    },
  },
  mounted() {
    this.update();
  },
  watch: {
    rootFiles: function (newValue, oldValue) {
      this.update();
    },
  },
  template: /*html*/ `
<div>
    <div class="row">
      <q-select
        dense filled option-label="path" class="col-4"
        @input="rootClick" v-model="root" :options="rootFiles"
      />

      <q-select
        dense filled class="col"
        v-model="folder" :options="root.folders"
        @input="indexFolder = getIndex(folder.path, root.folders)"
        :option-label="
          item => (!!item.path ? item.path.replace(root.path, '') : null)
        "
      />
    </div>

    <q-list bordered>
      <transition-group name="list-complete">
        <q-item v-ripple class="list-complete-item"
          v-for="file in folder.files" :key="file.name"
        >
          <q-item-section>
            <q-item-label>{{ file.name }}</q-item-label>
            <q-item-label caption>{{ formatSize(file.size) }}</q-item-label>
          </q-item-section>
          <q-item-section side>
            <div class=" q-gutter-sm text-white" style="display:flex;">
              <q-btn
                icon="icon-cloud_download" class="bg-primary"
                @click="downloadItem(getPath(file.name))"
              ></q-btn>
              <q-btn
                icon="icon-delete_forever" class="bg-warning"
                @click="deleteItem(getPath(file.name))"
              />
            </div>
          </q-item-section>
        </q-item>
      </transition-group>
    </q-list>

    <q-form @submit="submitFile()" class="q-mt-sm">
      <div style="display:flex;">
        <q-file dense filled style="flex-grow: 1;"
          v-model="file"
          lazy-rules label="Select a file"
          :rules="[val => !!val || 'Please select a file']"
        />
        <q-btn
          icon="icon-cloud_upload" color="primary" class="q-mb-lg q-ml-sm"
          type="submit"
        />
      </div>
    </q-form>
  </div>
  `,
});
