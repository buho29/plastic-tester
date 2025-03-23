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
          (item.data.find((entry) => entry.t === 0).d * 100) / item.length
        ).toFixed(3)
      );
      const fValues = this.rawData.map((item) =>
        (
          (item.data.find((entry) => entry.t === 0).f * 9.91) / item.area
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
            title: { display: true, text: "Distance(mm)" },
          },
          y: {
            title: { display: true, text: "Force(kg)" },
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
    makeDataSet( label, data, borderColor, backgroundColor, 
      pointStyle = true, fill = false) 
    {
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
          "Force (kg)",
          arrForce,
          "rgb(54, 162, 235)",
          "rgb(54, 162, 235)"
        ),
        this.makeDataSet(
          "Max (kg)",
          arrMax,
          "rgba(255, 99, 132,0.6)",
          "rgba(201, 220, 248, 0.2)",
          false,
          "+1"
        ),
        this.makeDataSet(
          "Min (kg)",
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
