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
        
        
        <q-btn push label="History" size="xl"  class="q-ma-md bthome" color="accent"
            glossy stack icon="icon-trending_up" to="history"/>
        
        
        <q-btn push label="Options" size="xl" class="q-ma-md bthome" color="info"
            glossy stack icon="icon-settings" to="options"/>
  
        
        <q-btn push label="System" size="xl" class="q-ma-md bthome" color="warning"
            glossy stack icon="icon-build" to="system"/>
        
        </div >
      </b-container>
    </q-page>
    `,
  };