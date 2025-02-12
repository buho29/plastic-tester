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
    <q-card-section>
      <slot></slot>
    </q-card-section>
  </q-card>
  `,
});

Vue.component("line-chart", {
  data: function () {
    return {
      chartData: null,
    };
  },
  props: ["data", "tags", "title", "labels"],
  extends: VueChartJs.Line,
  mixins: [VueChartJs.mixins.reactiveData],
  data() {
    return {
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: { yAxes: [{ ticks: { beginAtZero: false } }] },
      },
      colors: ["#31CCEC", "#008577", "#C10015"],
    };
  },
  methods: {
    update() {
      const steps = 10;
      const axisY = this.data.map((data) => data.t); // Extrae el tiempo
      const min = axisY.length > 0 ? Math.min(...axisY) : 100;
      const max = axisY.length > 0 ? Math.max(...axisY) : 2000;
      const step = (max - min) / (steps - 1);

      console.log(min, max, step, "teta");
      // creamos labels
      // TODO buscar min max y / 10 lineas
      let arrTime = [];
      for (let i = 0; i < steps; i++) {
        y = Math.round((step * i + min) * 100) / 100;
        arrTime.push(this.makeLabel(y));
      }

      // Inicializa arrData con arrays vacíos
      let arrData = this.tags.map(() => []);

      for (let i = 0; i < this.data.length; i++) {
        let item = this.data[i];
        for (let j = 0; j < this.tags.length; j++) {
          arrData[j].push(item[this.tags[j]]);
        }
      }

      let datasets = [];
      for (let i = 0; i < this.tags.length; i++) {
        datasets.push(this.makeDataSet(this.labels[i], arrData[i], i));
      }

      this.chartData = {
        labels: arrTime,
        datasets: datasets,
      };
    },
    makeDataSet(label, data, indexColor) {
      return {
        label: label,
        data: data,
        borderColor: this.colors[indexColor],
        fill: true,
        borderWidth: 2,
        backgroundColor: "#00000000",
        borderWidth: 2,
        type: "line",
        pointRadius: 2,
        lineTension: 0,
      };
    },
    makeLabel(value) {
      return value + "ms";
    },
  },
  watch: {
    data: function (newQuestion, oldQuestion) {
      this.update();
    },
  },
  mounted() {
    this.update();
    this.renderChart(this.chartData, this.options);
  },
});

Vue.component("bar-chart", {
  data: function () {
    return {
      chartData: null,
      labels: { f: "Kg", d: "mm" },
      colors: { f: "#31CCEC", d: "#C10015" },
      options: {
        scales: {
          yAxes: [
            {
              ticks: { beginAtZero: true },
              gridLines: { display: true },
            },
          ],
          xAxes: [
            {
              ticks: { beginAtZero: true },
              gridLines: { display: false },
            },
          ],
        },
        legend: { display: false },
        /*tooltips: {
                      enabled: true, mode: 'single',
                      callbacks: {
                          label: function(tooltipItems, data) {
                              return tooltipItems.yLabel;
                          }
                      }
                  },*/
        responsive: true,
        maintainAspectRatio: false,
        height: 200,
      },
    };
  },
  props: ["data", "prop"],
  extends: VueChartJs.Bar,
  mixins: [VueChartJs.mixins.reactiveData],
  methods: {
    update() {
      // creamos labels
      let arrTime = [];
      // Inicializa arrData con arrays vacíos
      let arrData = [];

      for (let i = 0; i < this.data.length; i++) {
        const result = this.data[i];
        arrTime.push(result.name);

        let axisY = result.data.map((m) => m[this.prop]); // Extrae
        let max = axisY.length > 0 ? Math.max(...axisY) : 1;
        arrData.push(max); /**/
      }

      console.log(arrTime, arrData, this.prop);

      let datasets = [];
      let label = this.labels[this.prop];
      let color = this.colors[this.prop];
      datasets.push(this.makeDataSet(label, arrData, color));

      this.chartData = {
        labels: arrTime,
        datasets: datasets,
      };
    },
    makeDataSet(label, data, color) {
      return {
        label: label,
        backgroundColor: color,
        pointBackgroundColor: "white",
        borderWidth: 1,
        pointBorderColor: "#249EBF",
        data: data,
      };
    },
  },
  watch: {
    data: function (newQuestion, oldQuestion) {
      this.update();
    },
    prop: function (newQuestion, oldQuestion) {
      this.update();
    },
  },
  mounted() {
    this.update();
    this.renderChart(this.chartData, this.options);
  },
});
